/***************************************************************************
    testqgsrelationreferencewidget.cpp
     --------------------------------------
    Date                 : 21 07 2017
    Copyright            : (C) 2017 Paul Blottiere
    Email                : paul dot blottiere at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "qgstest.h"
#include <QSignalSpy>

#include <editorwidgets/core/qgseditorwidgetregistry.h>
#include <qgsapplication.h>
#include "qgseditorwidgetwrapper.h"
#include <editorwidgets/qgsrelationreferencewidget.h>
#include <editorwidgets/qgsrelationreferencewidgetwrapper.h>
#include <editorwidgets/qgsrelationreferenceconfigdlg.h>
#include <qgsproject.h>
#include <qgsattributeform.h>
#include <qgsrelationmanager.h>
#include <attributetable/qgsattributetablefiltermodel.h>
#include "qgsfeaturelistcombobox.h"
#include "qgsfeaturefiltermodel.h"
#include "qgsgui.h"
#include "qgsmapcanvas.h"
#include "qgsvectorlayertools.h"
#include "qgsvectorlayertoolscontext.h"
#include "qgsadvanceddigitizingdockwidget.h"
#include "qgsmaptooldigitizefeature.h"

QStringList getComboBoxItems( const QComboBox *cb )
{
  QStringList items;
  for ( int i = 0; i < cb->count(); i++ )
    items << cb->itemText( i );

  return items;
}

class TestQgsRelationReferenceWidget : public QObject
{
    Q_OBJECT
  public:
    TestQgsRelationReferenceWidget() = default;

  private slots:
    void initTestCase();    // will be called before the first testfunction is executed.
    void cleanupTestCase(); // will be called after the last testfunction was executed.
    void init();            // will be called before each testfunction is executed.
    void cleanup();         // will be called after every testfunction.

    void testChainFilter();
    void testChainFilter_data();
    void testChainFilterFirstInit_data();
    void testChainFilterFirstInit();
    void testChainFilterRefreshed();
    void testChainFilterDeleteForeignKey();
    void testInvalidRelation();
    void testSetGetForeignKey();
    void testIdentifyOnMap();
    void testAddEntry();
    void testAddEntryNoGeom();
    void testDependencies(); // Test relation datasource, id etc. config storage
    void testSetFilterExpression();
    void testSetFilterExpressionWithOrClause();
    void testSetFilterExpressionWithCurrentValue();
    void testSetFilterExpressionWithParentValue();
    void testComboLimit();
    void testAllowNullDefault();
    void testSorting();

  private:
    QgsVectorLayer *mLayer1 = nullptr;
    QgsVectorLayer *mLayer2 = nullptr;
    std::unique_ptr<QgsRelation> mRelation;
    QgsMapCanvas *mMapCanvas = nullptr;
    QgsAdvancedDigitizingDockWidget *mCadWidget = nullptr;
};

void TestQgsRelationReferenceWidget::initTestCase()
{
  QgsApplication::init();
  QgsApplication::initQgis();
  QgsGui::editorWidgetRegistry()->initEditors();
  mMapCanvas = new QgsMapCanvas();
  mCadWidget = new QgsAdvancedDigitizingDockWidget( mMapCanvas );
}

void TestQgsRelationReferenceWidget::cleanupTestCase()
{
  delete mCadWidget;
  delete mMapCanvas;
  QgsApplication::exitQgis();
}

void TestQgsRelationReferenceWidget::init()
{
  // create layer
  mLayer1 = new QgsVectorLayer( QStringLiteral( "LineString?crs=epsg:3111&field=pk:int&field=fk:int" ), QStringLiteral( "vl1" ), QStringLiteral( "memory" ) );
  QgsProject::instance()->addMapLayer( mLayer1 );

  mLayer2 = new QgsVectorLayer( QStringLiteral( "LineString?field=pk:int&field=material:string&field=diameter:int&field=raccord:string&field=multiplicator:int" ), QStringLiteral( "vl2" ), QStringLiteral( "memory" ) );
  mLayer2->setDisplayExpression( QStringLiteral( "pk" ) );
  QgsProject::instance()->addMapLayer( mLayer2 );

  // create relation
  mRelation.reset( new QgsRelation() );
  mRelation->setId( QStringLiteral( "vl1.vl2" ) );
  mRelation->setName( QStringLiteral( "vl1.vl2" ) );
  mRelation->setReferencingLayer( mLayer1->id() );
  mRelation->setReferencedLayer( mLayer2->id() );
  mRelation->addFieldPair( QStringLiteral( "fk" ), QStringLiteral( "pk" ) );
  QVERIFY( mRelation->isValid() );
  QgsProject::instance()->relationManager()->addRelation( *mRelation );

  // add features
  QgsFeature ft0( mLayer1->fields() );
  ft0.setAttribute( QStringLiteral( "pk" ), 0 );
  ft0.setAttribute( QStringLiteral( "fk" ), 0 );
  mLayer1->startEditing();
  mLayer1->addFeature( ft0 );
  mLayer1->commitChanges();

  QgsFeature ft1( mLayer1->fields() );
  ft1.setAttribute( QStringLiteral( "pk" ), 1 );
  ft1.setAttribute( QStringLiteral( "fk" ), 1 );
  mLayer1->startEditing();
  mLayer1->addFeature( ft1 );
  mLayer1->commitChanges();

  QgsFeature ft2( mLayer2->fields() );
  ft2.setAttribute( QStringLiteral( "pk" ), 10 );
  ft2.setAttribute( QStringLiteral( "material" ), "iron" );
  ft2.setAttribute( QStringLiteral( "diameter" ), 120 );
  ft2.setAttribute( QStringLiteral( "raccord" ), "brides" );
  ft2.setAttribute( QStringLiteral( "multiplicator" ), 10 );
  mLayer2->startEditing();
  mLayer2->addFeature( ft2 );
  mLayer2->commitChanges();

  QgsFeature ft3( mLayer2->fields() );
  ft3.setAttribute( QStringLiteral( "pk" ), 11 );
  ft3.setAttribute( QStringLiteral( "material" ), "iron" );
  ft3.setAttribute( QStringLiteral( "diameter" ), 120 );
  ft3.setAttribute( QStringLiteral( "raccord" ), "sleeve" );
  ft3.setAttribute( QStringLiteral( "multiplicator" ), 1 );
  mLayer2->startEditing();
  mLayer2->addFeature( ft3 );
  mLayer2->commitChanges();

  QgsFeature ft4( mLayer2->fields() );
  ft4.setAttribute( QStringLiteral( "pk" ), 12 );
  ft4.setAttribute( QStringLiteral( "material" ), "steel" );
  ft4.setAttribute( QStringLiteral( "diameter" ), 120 );
  ft4.setAttribute( QStringLiteral( "raccord" ), "collar" );
  ft4.setAttribute( QStringLiteral( "multiplicator" ), 5 );
  mLayer2->startEditing();
  mLayer2->addFeature( ft4 );
  mLayer2->commitChanges();
}

void TestQgsRelationReferenceWidget::cleanup()
{
  QgsProject::instance()->clear();
}

void TestQgsRelationReferenceWidget::testChainFilter_data()
{
  QTest::addColumn<bool>( "allowNull" );

  QTest::newRow( "allowNull=true" ) << true;
  QTest::newRow( "allowNull=false" ) << false;
}

void TestQgsRelationReferenceWidget::testChainFilter()
{
  QFETCH( bool, allowNull );

  // init a relation reference widget
  QStringList filterFields = { "material", "diameter", "raccord" };

  QWidget parentWidget;
  QgsRelationReferenceWidget w( &parentWidget );

  QEventLoop loop;
  connect( qobject_cast<QgsFeatureFilterModel *>( w.mComboBox->model() ), &QgsFeatureFilterModel::filterJobCompleted, &loop, &QEventLoop::quit );

  w.setChainFilters( true );
  w.setFilterFields( filterFields );
  w.setRelation( *mRelation, allowNull );
  w.init();

  // check default status for comboboxes
  QList<QComboBox *> cbs = w.mFilterComboBoxes;
  QCOMPARE( cbs.count(), 3 );
  for ( const QComboBox *cb : std::as_const( cbs ) )
  {
    if ( cb->currentText() == QLatin1String( "raccord" ) )
      QCOMPARE( cb->count(), 5 );
    else if ( cb->currentText() == QLatin1String( "material" ) )
      QCOMPARE( cb->count(), 4 );
    else if ( cb->currentText() == QLatin1String( "diameter" ) )
      QCOMPARE( cb->count(), 3 );
  }

  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), allowNull ? QString( "NULL" ) : QString( "10" ) );

  // set first filter
  cbs[0]->setCurrentIndex( cbs[0]->findText( QStringLiteral( "iron" ) ) );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), allowNull ? QString( "NULL" ) : QString( "10" ) );

  cbs[1]->setCurrentIndex( cbs[1]->findText( QStringLiteral( "120" ) ) );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), allowNull ? QString( "NULL" ) : QString( "10" ) );

  for ( const QComboBox *cb : std::as_const( cbs ) )
  {
    if ( cb->itemText( 0 ) == QLatin1String( "material" ) )
      QCOMPARE( cb->count(), 4 );
    else if ( cb->itemText( 0 ) == QLatin1String( "diameter" ) )
      QCOMPARE( cb->count(), 2 );
    else if ( cb->itemText( 0 ) == QLatin1String( "raccord" ) )
    {
      QStringList items = getComboBoxItems( cb );

      QCOMPARE( cb->count(), 3 );
      QCOMPARE( items.contains( "collar" ), false );
      // collar should not be available in combobox as there's no existing
      // feature with the filter expression:
      // "material" ==  'iron' AND "diameter" == '120' AND "raccord" = 'collar'
    }
  }


  // set the filter for "raccord" and then reset filter for "diameter". As
  // chain filter is activated, the filter on "raccord" field should be reset

  cbs[0]->setCurrentIndex( 0 );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), allowNull ? QString( "NULL" ) : QString( "10" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "10" << "11" << "12" );

  if ( allowNull )
  {
    w.mComboBox->setCurrentIndex( w.mComboBox->findText( QStringLiteral( "10" ) ) );
    QCOMPARE( w.mComboBox->currentText(), QString( "10" ) );
    QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "10" << "11" << "12" );
  }

  cbs[0]->setCurrentIndex( cbs[0]->findText( "iron" ) );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), QString( "10" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "10" << "11" );

  // prefer 12 over NULL
  cbs[0]->setCurrentIndex( cbs[0]->findText( "steel" ) );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), QString( "12" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "12" );

  if ( allowNull )
  {
    w.mComboBox->setCurrentIndex( w.mComboBox->findText( QStringLiteral( "12" ) ) );
    QCOMPARE( w.mComboBox->currentText(), QString( "12" ) );
    QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "12" );
  }

  // reset IRON, prefer 10 over NULL
  cbs[0]->setCurrentIndex( cbs[0]->findText( "iron" ) );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), QString( "10" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "10" << "11" );

  if ( allowNull )
  {
    w.mComboBox->setCurrentIndex( w.mComboBox->findText( QStringLiteral( "10" ) ) );
    QCOMPARE( w.mComboBox->currentText(), QString( "10" ) );
    QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "10" << "11" );
  }

  cbs[1]->setCurrentIndex( cbs[1]->findText( "120" ) );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), QString( "10" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "10" << "11" );

  cbs[2]->setCurrentIndex( cbs[2]->findText( QStringLiteral( "brides" ) ) );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), QString( "10" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "10" );

  cbs[1]->setCurrentIndex( cbs[1]->findText( QStringLiteral( "diameter" ) ) );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), QString( "10" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "10" << "11" );

  // combobox should propose NULL (if allowNull is true), 10 and 11 because the filter is now:
  // "material" == 'iron'
  QCOMPARE( w.mComboBox->count(), allowNull ? 3 : 2 );

  // if there's no filter at all, all features' id should be proposed
  cbs[0]->setCurrentIndex( cbs[0]->findText( QStringLiteral( "material" ) ) );
  loop.exec();
  QCOMPARE( w.mComboBox->count(), allowNull ? 4 : 3 );
  QCOMPARE( w.mComboBox->currentText(), QString( "10" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "10" << "11" << "12" );

  // change item to check that currently selected item remains
  w.mComboBox->setCurrentIndex( w.mComboBox->findText( QStringLiteral( "11" ) ) );
  cbs[0]->setCurrentIndex( cbs[0]->findText( "iron" ) );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), QString( "11" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "10" << "11" );

  // reset all filter
  cbs[0]->setCurrentIndex( cbs[0]->findText( QStringLiteral( "material" ) ) );
  loop.exec();
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "10" << "11" << "12" );

  // set value with foreign key -> all the comboboxes matches feature values
  w.setForeignKeys( QVariantList() << "11" );
  loop.exec();
  QCOMPARE( cbs[0]->currentText(), QString( "iron" ) );
  QCOMPARE( cbs[1]->currentText(), QString( "120" ) );
  QCOMPARE( cbs[2]->currentText(), QString( "sleeve" ) );
  QCOMPARE( w.mComboBox->currentText(), QString( "11" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "11" );

  // remove filter on raccord
  cbs[2]->setCurrentIndex( cbs[2]->findText( "raccord" ) );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), QString( "11" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "10" << "11" );

  // change material, prever 12 over NULL
  cbs[0]->setCurrentIndex( cbs[0]->findText( QStringLiteral( "steel" ) ) );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), QString( "12" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "12" );
}

void TestQgsRelationReferenceWidget::testChainFilterFirstInit_data()
{
  QTest::addColumn<bool>( "allowNull" );

  QTest::newRow( "allowNull=true" ) << true;
  QTest::newRow( "allowNull=false" ) << false;
}

void TestQgsRelationReferenceWidget::testChainFilterFirstInit()
{
  QFETCH( bool, allowNull );

  // init a relation reference widget
  QStringList filterFields = { "material", "diameter", "raccord" };

  QWidget parentWidget;
  QgsRelationReferenceWidget w( &parentWidget );
  w.setChainFilters( true );
  w.setFilterFields( filterFields );
  w.setRelation( *mRelation, allowNull );
  w.init();

  // check default status for comboboxes
  QList<QComboBox *> cbs = w.mFilterComboBoxes;
  QCOMPARE( cbs.count(), 3 );
  for ( const QComboBox *cb : std::as_const( cbs ) )
  {
    if ( cb->currentText() == QLatin1String( "raccord" ) )
      QCOMPARE( cb->count(), 5 );
    else if ( cb->currentText() == QLatin1String( "material" ) )
      QCOMPARE( cb->count(), 4 );
    else if ( cb->currentText() == QLatin1String( "diameter" ) )
      QCOMPARE( cb->count(), 3 );
  }

  // set the filter for "raccord" and then reset filter for "diameter". As
  // chain filter is activated, the filter on "raccord" field should be reset
  QEventLoop loop;
  connect( qobject_cast<QgsFeatureFilterModel *>( w.mComboBox->model() ), &QgsFeatureFilterModel::filterJobCompleted, &loop, &QEventLoop::quit );

  // set value with foreign key -> all the comboboxes matches feature values
  w.setForeignKeys( QVariantList() << "11" );
  loop.exec();
  QCOMPARE( cbs[0]->currentText(), QString( "iron" ) );
  QCOMPARE( cbs[1]->currentText(), QString( "120" ) );
  QCOMPARE( cbs[2]->currentText(), QString( "sleeve" ) );
  QCOMPARE( w.mComboBox->currentText(), QString( "11" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "11" );

  // remove filter on raccord
  cbs[2]->setCurrentIndex( cbs[2]->findText( "raccord" ) );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), QString( "11" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "10" << "11" );

  // change material prever 12 over NULL
  cbs[0]->setCurrentIndex( cbs[0]->findText( QStringLiteral( "steel" ) ) );
  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), QString( "12" ) );
  QCOMPARE( getComboBoxItems( w.mComboBox ), ( allowNull ? QStringList() << "NULL" : QStringList() ) << "12" );
}


void TestQgsRelationReferenceWidget::testChainFilterRefreshed()
{
  // init a relation reference widget
  QStringList filterFields = { "material", "diameter", "raccord" };

  QgsRelationReferenceWidget w( new QWidget() );
  w.setChainFilters( true );
  w.setFilterFields( filterFields );
  w.setRelation( *mRelation, true );
  w.init();

  // check default status for comboboxes
  QList<QComboBox *> cbs = w.mFilterComboBoxes;
  QCOMPARE( cbs.count(), 3 );
  QCOMPARE( cbs[0]->currentText(), QString( "material" ) );
  QCOMPARE( cbs[1]->currentText(), QString( "diameter" ) );
  QCOMPARE( cbs[2]->currentText(), QString( "raccord" ) );

  // update foreign key
  w.setForeignKeys( QVariantList() << QVariant( 12 ) );
  QCOMPARE( cbs[0]->currentText(), QString( "steel" ) );
  QCOMPARE( cbs[1]->currentText(), QString( "120" ) );
  QCOMPARE( cbs[2]->currentText(), QString( "collar" ) );

  w.setForeignKeys( QVariantList() << QVariant( 10 ) );
  QCOMPARE( cbs[0]->currentText(), QString( "iron" ) );
  QCOMPARE( cbs[1]->currentText(), QString( "120" ) );
  QCOMPARE( cbs[2]->currentText(), QString( "brides" ) );

  w.setForeignKeys( QVariantList() << QVariant( 11 ) );
  QCOMPARE( cbs[0]->currentText(), QString( "iron" ) );
  QCOMPARE( cbs[1]->currentText(), QString( "120" ) );
  QCOMPARE( cbs[2]->currentText(), QString( "sleeve" ) );
}

void TestQgsRelationReferenceWidget::testChainFilterDeleteForeignKey()
{
  // init a relation reference widget
  QStringList filterFields = { "material", "diameter", "raccord" };

  QgsRelationReferenceWidget w( new QWidget() );
  w.setChainFilters( true );
  w.setFilterFields( filterFields );
  w.setRelation( *mRelation, true );
  w.init();

  // check the default status of filter comboboxes
  QList<QComboBox *> cbs = w.mFilterComboBoxes;

  QCOMPARE( cbs[0]->currentText(), QString( "material" ) );
  QCOMPARE( cbs[0]->isEnabled(), true );

  QCOMPARE( cbs[1]->currentText(), QString( "diameter" ) );
  QCOMPARE( cbs[1]->isEnabled(), false );

  QCOMPARE( cbs[2]->currentText(), QString( "raccord" ) );
  QCOMPARE( cbs[2]->isEnabled(), false );

  // set a foreign key
  w.setForeignKeys( QVariantList() << QVariant( 11 ) );

  QCOMPARE( cbs[0]->currentText(), QString( "iron" ) );
  QCOMPARE( cbs[1]->currentText(), QString( "120" ) );
  QCOMPARE( cbs[2]->currentText(), QString( "sleeve" ) );

  // delete the foreign key
  w.deleteForeignKeys();

  QCOMPARE( cbs[0]->currentText(), QString( "material" ) );
  QCOMPARE( cbs[0]->isEnabled(), true );

  QCOMPARE( cbs[1]->currentText(), QString( "diameter" ) );
  QCOMPARE( cbs[1]->isEnabled(), false );

  QCOMPARE( cbs[2]->currentText(), QString( "raccord" ) );
  QCOMPARE( cbs[2]->isEnabled(), false );

  // set a foreign key
  w.setForeignKeys( QVariantList() << QVariant( 11 ) );

  QCOMPARE( cbs[0]->currentText(), QString( "iron" ) );
  QCOMPARE( cbs[1]->currentText(), QString( "120" ) );
  QCOMPARE( cbs[2]->currentText(), QString( "sleeve" ) );

  // set a null foreign key
  w.setForeignKeys( QVariantList() << QgsVariantUtils::createNullVariant( QMetaType::Type::Int ) );
  QCOMPARE( cbs[0]->currentText(), QString( "material" ) );
  QCOMPARE( cbs[0]->isEnabled(), true );
  QCOMPARE( cbs[1]->currentText(), QString( "diameter" ) );
  QCOMPARE( cbs[1]->isEnabled(), false );
  QCOMPARE( cbs[2]->currentText(), QString( "raccord" ) );
  QCOMPARE( cbs[2]->isEnabled(), false );
}

void TestQgsRelationReferenceWidget::testInvalidRelation()
{
  QgsVectorLayer vl( QStringLiteral( "LineString?crs=epsg:3111&field=pk:int&field=fk:int" ), QStringLiteral( "vl1" ), QStringLiteral( "memory" ) );

  QgsRelationReferenceWidget editor( new QWidget() );

  // initWidget with an invalid relation
  QgsRelationReferenceWidgetWrapper ww( &vl, 10, &editor, mMapCanvas, nullptr, nullptr );

  QgsAttributeEditorContext context = ww.context();
  context.setCadDockWidget( mCadWidget );
  ww.setContext( context );

  ww.initWidget( nullptr );
}

void TestQgsRelationReferenceWidget::testSetGetForeignKey()
{
  QWidget parentWidget;
  QgsRelationReferenceWidget w( &parentWidget );

  w.setRelation( *mRelation, true );
  w.init();

  QSignalSpy spy( &w, &QgsRelationReferenceWidget::foreignKeysChanged );
  QEventLoop loop;

  w.setForeignKeys( QVariantList() << QVariant() );

  QTimer::singleShot( 1000, &loop, &QEventLoop::quit );
  loop.exec();

  QVERIFY( w.foreignKeys().at( 0 ).isNull() );
  QVERIFY( w.foreignKeys().at( 0 ).isValid() );
  QCOMPARE( spy.count(), 1 );

  w.setForeignKeys( QVariantList() << 12 );

  QTimer::singleShot( 1000, &loop, &QEventLoop::quit );
  loop.exec();

  QCOMPARE( w.foreignKeys().at( 0 ), QVariant( 12 ) );
  QCOMPARE( w.mComboBox->currentText(), QStringLiteral( "12" ) );
  QCOMPARE( spy.count(), 2 );

  w.setForeignKeys( QVariantList() << 11 );

  QTimer::singleShot( 1000, &loop, &QEventLoop::quit );
  loop.exec();

  QCOMPARE( w.foreignKeys().at( 0 ), QVariant( 11 ) );
  QCOMPARE( w.mComboBox->currentText(), QStringLiteral( "11" ) );
  QCOMPARE( spy.count(), 3 );

  w.setForeignKeys( QVariantList() << 0 );

  QTimer::singleShot( 1000, &loop, &QEventLoop::quit );
  loop.exec();

  QCOMPARE( w.foreignKeys().at( 0 ), QVariant( 0 ) );
  QCOMPARE( w.mComboBox->currentText(), QStringLiteral( "(0)" ) );
  QCOMPARE( spy.count(), 4 );

  w.setForeignKeys( QVariantList() << QVariant() );

  QTimer::singleShot( 1000, &loop, &QEventLoop::quit );
  loop.exec();

  QVERIFY( w.foreignKeys().at( 0 ).isNull() );
  QVERIFY( w.foreignKeys().at( 0 ).isValid() );
  QCOMPARE( spy.count(), 5 );
}

// Test issue https://github.com/qgis/QGIS/issues/29884
// Relation reference widget wrong feature when "on map identification"
void TestQgsRelationReferenceWidget::testIdentifyOnMap()
{
  QWidget parentWidget;
  QgsRelationReferenceWidget w( &parentWidget );
  QVERIFY( mLayer1->startEditing() );
  w.setRelation( *mRelation, true );
  w.setAllowMapIdentification( true );
  w.init();
  QEventLoop loop;
  // Populate model (I tried to listen to signals but the module reload() runs twice
  // (the first load triggers a second one which does the population of the combo)
  // and I haven't fin a way to properly wait for it.
  QTimer::singleShot( 300, this, [&] { loop.quit(); } );
  loop.exec();
  QgsFeature feature;
  mLayer2->getFeatures( QStringLiteral( "pk = %1" ).arg( 11 ) ).nextFeature( feature );
  QVERIFY( feature.isValid() );
  QCOMPARE( feature.attribute( QStringLiteral( "pk" ) ).toInt(), 11 );
  w.featureIdentified( feature );
  QCOMPARE( w.mComboBox->currentData( Qt::DisplayRole ).toInt(), 11 );

  mLayer2->getFeatures( QStringLiteral( "pk = %1" ).arg( 10 ) ).nextFeature( feature );
  QVERIFY( feature.isValid() );
  QCOMPARE( feature.attribute( QStringLiteral( "pk" ) ).toInt(), 10 );
  w.featureIdentified( feature );
  QCOMPARE( w.mComboBox->currentData( Qt::DisplayRole ).toInt(), 10 );

  w.setReadOnlySelector( true );
  QVERIFY( !w.mComboBox->isEnabled() );

  mLayer1->rollBack();
}

// Monkey patch gui vector layer tool in order to simple add a new feature in
// referenced layer
class DummyVectorLayerTools : public QgsVectorLayerTools // clazy:exclude=missing-qobject-macro
{
    bool addFeatureV2( QgsVectorLayer *layer, const QgsAttributeMap &, const QgsGeometry &, QgsFeature *feat, const QgsVectorLayerToolsContext &context ) const override
    {
      Q_UNUSED( context );
      feat->setAttribute( QStringLiteral( "pk" ), 13 );
      feat->setAttribute( QStringLiteral( "material" ), QStringLiteral( "steel" ) );
      feat->setAttribute( QStringLiteral( "diameter" ), 140 );
      feat->setAttribute( QStringLiteral( "raccord" ), "collar" );
      layer->addFeature( *feat );
      return true;
    }

    bool startEditing( QgsVectorLayer * ) const override { return true; }

    bool stopEditing( QgsVectorLayer *, bool = true ) const override { return true; }

    bool saveEdits( QgsVectorLayer * ) const override { return true; }
};

void TestQgsRelationReferenceWidget::testAddEntry()
{
  // check that a new added entry in referenced layer populate correctly the
  // referencing combobox
  QgsMapCanvas canvas;
  QgsRelationReferenceWidget w( &canvas );
  QVERIFY( mLayer1->startEditing() );
  w.setRelation( *mRelation, true );
  w.init();

  QgsAdvancedDigitizingDockWidget cadDockWidget( &canvas );
  QgsAttributeEditorContext context;
  DummyVectorLayerTools tools;
  context.setVectorLayerTools( &tools );
  context.setCadDockWidget( &cadDockWidget );
  w.setEditorContext( context, &canvas, nullptr );
  w.addEntry();

  QVERIFY( w.mCurrentMapTool );
  QgsFeature feat( mLayer1->fields() );
  emit w.mMapToolDigitize->digitizingCompleted( feat );

  QCOMPARE( w.mComboBox->identifierValues().at( 0 ).toInt(), 13 );
}

void TestQgsRelationReferenceWidget::testAddEntryNoGeom()
{
  QgsVectorLayer *layer1 = new QgsVectorLayer( QStringLiteral( "Point?crs=epsg:3111&field=pk:int&field=fk:int" ), QStringLiteral( "vl1" ), QStringLiteral( "memory" ) );
  QgsProject::instance()->addMapLayer( layer1 );

  QgsVectorLayer *layer2 = new QgsVectorLayer( QStringLiteral( "None?field=pk:int&field=material:string" ), QStringLiteral( "vl2" ), QStringLiteral( "memory" ) );
  QgsProject::instance()->addMapLayer( layer2 );

  // create relation
  QgsRelation mRelation;
  mRelation.setId( QStringLiteral( "vl1.vl2" ) );
  mRelation.setName( QStringLiteral( "vl1.vl2" ) );
  mRelation.setReferencingLayer( layer1->id() );
  mRelation.setReferencedLayer( layer2->id() );
  mRelation.addFieldPair( QStringLiteral( "fk" ), QStringLiteral( "pk" ) );
  QVERIFY( mRelation.isValid() );
  QgsProject::instance()->relationManager()->addRelation( mRelation );

  // add feature
  QgsFeature ft0( layer1->fields() );
  ft0.setAttribute( QStringLiteral( "pk" ), 0 );
  ft0.setAttribute( QStringLiteral( "fk" ), 0 );
  layer1->startEditing();
  layer1->addFeature( ft0 );
  layer1->commitChanges();

  // check that a new added entry in referenced layer populate correctly the
  // referencing combobox
  QgsMapCanvas canvas;
  QgsRelationReferenceWidget w( &canvas );
  QVERIFY( layer1->startEditing() );
  w.setRelation( mRelation, true );
  w.init();

  QgsAdvancedDigitizingDockWidget cadDockWidget( &canvas );
  QgsAttributeEditorContext context;
  DummyVectorLayerTools tools;
  context.setVectorLayerTools( &tools );
  context.setCadDockWidget( &cadDockWidget );
  w.setEditorContext( context, &canvas, nullptr );
  w.addEntry();

  QVERIFY( !w.mCurrentMapTool );

  QCOMPARE( w.mComboBox->identifierValues().at( 0 ).toInt(), 13 );
}

void TestQgsRelationReferenceWidget::testDependencies()
{
  QgsVectorLayer *layer1 = new QgsVectorLayer( QStringLiteral( "Point?crs=epsg:3111&field=pk:int&field=fk:int" ), QStringLiteral( "vl1" ), QStringLiteral( "memory" ) );
  QgsProject::instance()->addMapLayer( layer1 );

  QgsVectorLayer *layer2 = new QgsVectorLayer( QStringLiteral( "None?field=pk:int&field=material:string" ), QStringLiteral( "vl2" ), QStringLiteral( "memory" ) );
  QgsProject::instance()->addMapLayer( layer2 );

  // create relation
  QgsRelation mRelation;
  mRelation.setId( QStringLiteral( "vl1.vl2" ) );
  mRelation.setName( QStringLiteral( "vl1.vl2" ) );
  mRelation.setReferencingLayer( layer1->id() );
  mRelation.setReferencedLayer( layer2->id() );
  mRelation.addFieldPair( QStringLiteral( "fk" ), QStringLiteral( "pk" ) );
  QVERIFY( mRelation.isValid() );
  QgsProject::instance()->relationManager()->addRelation( mRelation );

  // check that a new added entry in referenced layer populate correctly
  // widget config
  QgsMapCanvas canvas;
  QgsRelationReferenceWidget w( &canvas );
  w.setRelation( mRelation, true );
  w.init();

  QCOMPARE( w.referencedLayerId(), layer2->id() );
  QCOMPARE( w.referencedLayerName(), layer2->name() );
  QCOMPARE( w.referencedLayerDataSource(), layer2->publicSource() );
  QCOMPARE( w.referencedLayerProviderKey(), layer2->providerType() );
}

void TestQgsRelationReferenceWidget::testSetFilterExpression()
{
  // init a relation reference widget
  QStringList filterFields = { "material", "diameter", "raccord" };

  QWidget parentWidget;
  QgsRelationReferenceWidget w( &parentWidget );

  QEventLoop loop;
  connect( qobject_cast<QgsFeatureFilterModel *>( w.mComboBox->model() ), &QgsFeatureFilterModel::filterJobCompleted, &loop, &QEventLoop::quit );

  w.setChainFilters( true );
  w.setFilterFields( filterFields );
  w.setRelation( *mRelation, true );
  w.setFilterExpression( QStringLiteral( " \"material\" = 'iron' " ) );
  w.init();

  loop.exec();
  QCOMPARE( w.mComboBox->currentText(), QStringLiteral( "NULL" ) );
  // in case there is no filter, the number of filtered features will be 4
  QCOMPARE( w.mComboBox->count(), 3 );
}

void TestQgsRelationReferenceWidget::testSetFilterExpressionWithOrClause()
{
  // init a relation reference widget
  QStringList filterFields = { "material", "diameter", "raccord" };

  QWidget parentWidget;
  QgsRelationReferenceWidget w( &parentWidget );

  QEventLoop loop;
  connect( qobject_cast<QgsFeatureFilterModel *>( w.mComboBox->model() ), &QgsFeatureFilterModel::filterJobCompleted, &loop, &QEventLoop::quit );

  w.setChainFilters( true );
  w.setFilterFields( filterFields );
  w.setRelation( *mRelation, true );
  w.setFilterExpression( QStringLiteral( " \"raccord\" = 'sleeve' OR FALSE " ) );
  w.init();

  loop.exec();

  // in case there is no filter, the number of filtered features will be 4
  QCOMPARE( w.mComboBox->count(), 2 );

  QList<QComboBox *> cbs = w.mFilterComboBoxes;
  cbs[0]->setCurrentIndex( cbs[0]->findText( "steel" ) );

  loop.exec();

  QCOMPARE( w.mComboBox->currentText(), QStringLiteral( "NULL" ) );
  // in case there is no field filter, the number of filtered features will be 2
  QCOMPARE( w.mComboBox->count(), 1 );
}

void TestQgsRelationReferenceWidget::testSetFilterExpressionWithCurrentValue()
{
  QWidget parentWidget;
  QgsRelationReferenceWidget w( &parentWidget );

  QEventLoop loop;
  connect( qobject_cast<QgsFeatureFilterModel *>( w.mComboBox->model() ), &QgsFeatureFilterModel::filterJobCompleted, &loop, &QEventLoop::quit );

  QgsFields fields(
    QList<QgsField> {
      QgsField( QStringLiteral( "field1" ), QMetaType::Type::QString ),
      QgsField( QStringLiteral( "field2" ), QMetaType::Type::Int ),
      QgsField( QStringLiteral( "field3" ), QMetaType::Type::Double ),
    }
  );
  QgsFeature feature( fields );
  feature.setAttribute( QStringLiteral( "field1" ), QStringLiteral( "iron" ) );
  feature.setAttribute( QStringLiteral( "field2" ), 1 );
  feature.setAttribute( QStringLiteral( "field3" ), 1.0 );

  w.setRelation( *mRelation, true );
  w.setFormFeature( feature );
  w.setFilterExpression( QStringLiteral( "\"material\" = current_value('field1')" ) );
  w.init();

  loop.exec();

  QCOMPARE( w.mComboBox->count(), 3 );
}

void TestQgsRelationReferenceWidget::testSetFilterExpressionWithParentValue()
{
  QWidget parentWidget;
  QgsRelationReferenceWidget w( &parentWidget );

  QEventLoop loop;
  connect( qobject_cast<QgsFeatureFilterModel *>( w.mComboBox->model() ), &QgsFeatureFilterModel::filterJobCompleted, &loop, &QEventLoop::quit );

  QgsFields fields(
    QList<QgsField> {
      QgsField( QStringLiteral( "field1" ), QMetaType::Type::QString ),
      QgsField( QStringLiteral( "field2" ), QMetaType::Type::Int ),
      QgsField( QStringLiteral( "field3" ), QMetaType::Type::Double ),
    }
  );
  QgsFeature feature( fields );
  feature.setAttribute( QStringLiteral( "field1" ), QStringLiteral( "iron" ) );
  feature.setAttribute( QStringLiteral( "field2" ), 1 );
  feature.setAttribute( QStringLiteral( "field3" ), 1.0 );

  w.setRelation( *mRelation, true );
  w.setParentFormFeature( feature );
  w.setFilterExpression( QStringLiteral( "\"material\" = current_parent_value('field1')" ) );
  w.init();

  loop.exec();

  QCOMPARE( w.mComboBox->count(), 3 );
}

void TestQgsRelationReferenceWidget::testComboLimit()
{
  // create layer
  QgsVectorLayer *childLayer = new QgsVectorLayer( QStringLiteral( "LineString?crs=epsg:3111&field=pk:int&field=fk:int" ), QStringLiteral( "vlchild" ), QStringLiteral( "memory" ) );
  QgsProject::instance()->addMapLayer( childLayer );

  QgsVectorLayer *parentLayer = new QgsVectorLayer( QStringLiteral( "LineString?field=pk:int&field=material:string&field=diameter:int&field=raccord:string" ), QStringLiteral( "vlparent" ), QStringLiteral( "memory" ) );
  QgsProject::instance()->addMapLayer( parentLayer );

  // create relation
  QgsRelation mRelation;
  mRelation.setId( QStringLiteral( "vlchild.vlparent" ) );
  mRelation.setName( QStringLiteral( "vlchild.vlparent" ) );
  mRelation.setReferencingLayer( childLayer->id() );
  mRelation.setReferencedLayer( parentLayer->id() );
  mRelation.addFieldPair( QStringLiteral( "fk" ), QStringLiteral( "pk" ) );
  QVERIFY( mRelation.isValid() );
  QgsProject::instance()->relationManager()->addRelation( mRelation );

  // add features
  QgsFeature ft0( childLayer->fields() );
  ft0.setAttribute( QStringLiteral( "pk" ), 0 );
  ft0.setAttribute( QStringLiteral( "fk" ), 0 );
  childLayer->startEditing();
  childLayer->addFeature( ft0 );
  childLayer->commitChanges();

  QgsFeature ft1( childLayer->fields() );
  ft1.setAttribute( QStringLiteral( "pk" ), 1 );
  ft1.setAttribute( QStringLiteral( "fk" ), 1 );
  childLayer->startEditing();
  childLayer->addFeature( ft1 );
  childLayer->commitChanges();

  for ( int i = 0; i < 200; i++ )
  {
    QgsFeature ft( parentLayer->fields() );
    ft.setAttribute( QStringLiteral( "pk" ), i );
    ft.setAttribute( QStringLiteral( "material" ), QStringLiteral( "material %1" ).arg( i ) );
    ft.setAttribute( QStringLiteral( "diameter" ), 100 );
    ft.setAttribute( QStringLiteral( "raccord" ), QStringLiteral( "raccord %1" ).arg( i ) );
    parentLayer->startEditing();
    parentLayer->addFeature( ft );
    parentLayer->commitChanges();
  }

  QCOMPARE( parentLayer->featureCount(), 200 );

  QWidget parentWidget;
  QgsRelationReferenceWidget w( &parentWidget );
  QEventLoop loop;
  connect( qobject_cast<QgsFeatureFilterModel *>( w.mComboBox->model() ), &QgsFeatureFilterModel::filterJobCompleted, &loop, &QEventLoop::quit );
  w.setRelation( mRelation, false );
  loop.exec();
  QVERIFY( w.relation().isValid() );

  // check fetch limit of combobox directly
  QSignalSpy spy( w.mComboBox, &QgsFeatureListComboBox::modelUpdated );

  w.mComboBox->setFetchLimit( 20 );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 20 );

  w.mComboBox->setFetchLimit( -1 );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 200 );

  w.mComboBox->setFetchLimit( 120 );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 120 );

  w.mComboBox->setFetchLimit( 0 );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 200 );

  w.mComboBox->setFetchLimit( 300 );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 200 );

  // check the setting in relation reference
  w.setFetchLimit( 22 );
  w.init();
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 22 );

  w.setFetchLimit( -1 );
  w.init();
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 200 );

  w.setFetchLimit( 122 );
  w.init();
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 122 );

  w.setFetchLimit( 0 );
  w.setRelation( mRelation, false );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 200 );

  w.setFetchLimit( 300 );
  w.setRelation( mRelation, true );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 201 );
}

void TestQgsRelationReferenceWidget::testAllowNullDefault()
{
  // Create parent and child layers
  QgsVectorLayer *parentLayer = new QgsVectorLayer( QStringLiteral( "LineString?field=pk:int&field=material:string&field=diameter:int&field=raccord:string" ), QStringLiteral( "vlparent" ), QStringLiteral( "memory" ) );
  QgsProject::instance()->addMapLayer( parentLayer );

  QgsVectorLayer *childLayer = new QgsVectorLayer( QStringLiteral( "LineString?crs=epsg:3111&field=pk:int&field=fk:int" ), QStringLiteral( "vlchild" ), QStringLiteral( "memory" ) );
  QgsProject::instance()->addMapLayer( childLayer );

  QgsRelation relation;
  relation.setId( QStringLiteral( "vlchild.vlparent" ) );
  relation.setName( QStringLiteral( "vlchild.vlparent" ) );
  relation.setReferencingLayer( childLayer->id() );
  relation.setReferencedLayer( parentLayer->id() );
  relation.addFieldPair( QStringLiteral( "fk" ), QStringLiteral( "pk" ) );
  QVERIFY( relation.isValid() );

  QgsProject::instance()->relationManager()->addRelation( relation );

  // Test the config dialog
  std::unique_ptr<QgsRelationReferenceConfigDlg> dlg;
  dlg.reset( static_cast<QgsRelationReferenceConfigDlg *>( QgsGui::editorWidgetRegistry()->createConfigWidget( QStringLiteral( "RelationReference" ), childLayer, 1, nullptr ) ) );
  QVERIFY( dlg );

  // Check that "Allow NULL" was not set by config
  QCOMPARE( dlg->mAllowNullWasSetByConfig, false );

  // Check the default value of "Allow NULL" checkbox
  QCOMPARE( dlg->mCbxAllowNull->isChecked(), true );

  // Set "Allow NULL" by config
  QVariantMap config = dlg->config();
  config["AllowNULL"] = false;
  dlg->setConfig( config );

  QCOMPARE( dlg->mAllowNullWasSetByConfig, true );
  QCOMPARE( dlg->mCbxAllowNull->isChecked(), false );
}

void TestQgsRelationReferenceWidget::testSorting()
{
  QWidget parentWidget;
  QgsRelationReferenceWidget w( &parentWidget );
  w.init();

  // check fetch limit of combobox directly
  QSignalSpy spy( w.mComboBox, &QgsFeatureListComboBox::modelUpdated );

  // no sort setting - orders according display expression what is the pk
  w.setRelation( *mRelation, false );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 3 );
  QCOMPARE( w.mComboBox->itemData( 0, Qt::DisplayRole ).toString(), "10" );
  QCOMPARE( w.mComboBox->itemData( 1, Qt::DisplayRole ).toString(), "11" );
  QCOMPARE( w.mComboBox->itemData( 2, Qt::DisplayRole ).toString(), "12" );

  // ... and order descent
  w.setSortOrder( Qt::DescendingOrder );
  w.setRelation( *mRelation, false );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 3 );
  QCOMPARE( w.mComboBox->itemData( 0, Qt::DisplayRole ).toString(), "12" );
  QCOMPARE( w.mComboBox->itemData( 1, Qt::DisplayRole ).toString(), "11" );
  QCOMPARE( w.mComboBox->itemData( 2, Qt::DisplayRole ).toString(), "10" );

  // no sort setting - orders according display expression what is set to "'l2 '||raccord"
  w.mComboBox->setDisplayExpression( QStringLiteral( "'l2 '||raccord" ) );
  w.setSortOrder( Qt::AscendingOrder );
  w.setRelation( *mRelation, false );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 3 );
  QCOMPARE( w.mComboBox->itemData( 0, Qt::DisplayRole ).toString(), "l2 brides" );
  QCOMPARE( w.mComboBox->itemData( 1, Qt::DisplayRole ).toString(), "l2 collar" );
  QCOMPARE( w.mComboBox->itemData( 2, Qt::DisplayRole ).toString(), "l2 sleeve" );

  // ... and order descent
  w.setSortOrder( Qt::DescendingOrder );
  w.setRelation( *mRelation, false );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 3 );
  QCOMPARE( w.mComboBox->itemData( 0, Qt::DisplayRole ).toString(), "l2 sleeve" );
  QCOMPARE( w.mComboBox->itemData( 1, Qt::DisplayRole ).toString(), "l2 collar" );
  QCOMPARE( w.mComboBox->itemData( 2, Qt::DisplayRole ).toString(), "l2 brides" );

  //sorting setting - orders according orderby expression what is set to "'test '||(diameter * multiplicator)"
  //results in "test 1200" (l2 brides), "test 120" (l2 sleeve), "test 600" (l2 collar)
  //where this order is not numerical and has the 1200 before 600
  w.setOrderExpression( QStringLiteral( "'test '||(diameter * multiplicator)" ) );
  w.setSortOrder( Qt::AscendingOrder );
  w.setRelation( *mRelation, false );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 3 );
  QCOMPARE( w.mComboBox->itemData( 0, Qt::DisplayRole ).toString(), "l2 sleeve" ); //test 120
  QCOMPARE( w.mComboBox->itemData( 1, Qt::DisplayRole ).toString(), "l2 brides" ); //test 1200
  QCOMPARE( w.mComboBox->itemData( 2, Qt::DisplayRole ).toString(), "l2 collar" ); //test 600

  // ... and order descent
  w.setSortOrder( Qt::DescendingOrder );
  w.setRelation( *mRelation, false );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 3 );
  QCOMPARE( w.mComboBox->itemData( 0, Qt::DisplayRole ).toString(), "l2 collar" ); //test 600
  QCOMPARE( w.mComboBox->itemData( 1, Qt::DisplayRole ).toString(), "l2 brides" ); //test 1200
  QCOMPARE( w.mComboBox->itemData( 2, Qt::DisplayRole ).toString(), "l2 sleeve" ); //test 120


  //numeric sorting setting - orders according orderby expression what is set to "multiplicator"
  //results in "1" (l2 sleeve), "5" (l2 collar), "10" (l2 brides)
  //where this order should be made numerical and have 5 before 10
  w.setOrderExpression( QStringLiteral( "multiplicator" ) );
  w.setSortOrder( Qt::AscendingOrder );
  w.setRelation( *mRelation, false );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 3 );
  QCOMPARE( w.mComboBox->itemData( 0, Qt::DisplayRole ).toString(), "l2 sleeve" ); // 1
  QCOMPARE( w.mComboBox->itemData( 1, Qt::DisplayRole ).toString(), "l2 collar" ); // 5
  QCOMPARE( w.mComboBox->itemData( 2, Qt::DisplayRole ).toString(), "l2 brides" ); //10

  // ... and order descent
  w.setSortOrder( Qt::DescendingOrder );
  w.setRelation( *mRelation, false );
  spy.wait();
  QCOMPARE( w.mComboBox->count(), 3 );
  QCOMPARE( w.mComboBox->itemData( 0, Qt::DisplayRole ).toString(), "l2 brides" ); //10
  QCOMPARE( w.mComboBox->itemData( 1, Qt::DisplayRole ).toString(), "l2 collar" ); // 5
  QCOMPARE( w.mComboBox->itemData( 2, Qt::DisplayRole ).toString(), "l2 sleeve" ); // 1
}

QGSTEST_MAIN( TestQgsRelationReferenceWidget )
#include "testqgsrelationreferencewidget.moc"
