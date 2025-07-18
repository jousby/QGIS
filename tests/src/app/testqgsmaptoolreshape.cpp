/***************************************************************************
     testqgsmaptoolreshape.cpp
     --------------------------------
    Date                 : 2017-12-1
    Copyright            : (C) 2017 by Paul Blottiere
    Email                : paul.blottiere@oslandia.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgstest.h"

#include "qgisapp.h"
#include "qgsgeometry.h"
#include "qgsmapcanvas.h"
#include "qgssnappingconfig.h"
#include "qgssnappingutils.h"
#include "qgsmaptoolreshape.h"
#include "qgsproject.h"
#include "qgssettingsregistrycore.h"
#include "qgsvectorlayer.h"
#include "qgsmapcanvastracer.h"
#include "testqgsmaptoolutils.h"

#include <QSignalSpy>


/**
 * \ingroup UnitTests
 * This is a unit test for the vertex tool
 */
class TestQgsMapToolReshape : public QObject
{
    Q_OBJECT
  public:
    TestQgsMapToolReshape();

  private slots:
    void initTestCase();    // will be called before the first testfunction is executed.
    void cleanupTestCase(); // will be called after the last testfunction was executed.

    void testReshapeNotEnoughPoints();
    void testReshapeNoChange();
    void testReshapeZ();
    void testTopologicalEditing();
    void testTopologicalEditingNoSnap();
    void testAvoidIntersectionAndTopoEdit();
    void testAvoidIntersectionAndTopoEditSameLayer();
    void testAvoidIntersectionAndTopoEditSameLayerSelection();
    void reshapeWithBindingLine();
    void testWithTracing();
    void testKeepDirection();
    void testWithSnapToSegment();

  private:
    QgisApp *mQgisApp = nullptr;
    QgsMapCanvas *mCanvas = nullptr;
    QgsMapToolReshape *mCaptureTool = nullptr;
    QgsVectorLayer *mLayerLineZ = nullptr;
    QgsVectorLayer *mLayerPointZ = nullptr;
    QgsVectorLayer *mLayerPolygonZ = nullptr;
    QgsVectorLayer *mLayerTopo = nullptr;
    QgsVectorLayer *mLayerTopo2 = nullptr;
    QgsVectorLayer *mLayerLine = nullptr;
};

TestQgsMapToolReshape::TestQgsMapToolReshape() = default;


//runs before all tests
void TestQgsMapToolReshape::initTestCase()
{
  qDebug() << "TestMapToolCapture::initTestCase()";
  // init QGIS's paths - true means that all path will be inited from prefix
  QgsApplication::init();
  QgsApplication::initQgis();

  // Set up the QSettings environment
  QCoreApplication::setOrganizationName( QStringLiteral( "QGIS" ) );
  QCoreApplication::setOrganizationDomain( QStringLiteral( "qgis.org" ) );
  QCoreApplication::setApplicationName( QStringLiteral( "QGIS-TEST" ) );

  mQgisApp = new QgisApp();

  mCanvas = new QgsMapCanvas();

  mCanvas->setDestinationCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:3946" ) ) );

  mCanvas->setFrameStyle( QFrame::NoFrame );
  mCanvas->resize( 512, 512 );
  mCanvas->setExtent( QgsRectangle( 0, 0, 8, 8 ) );
  mCanvas->show(); // to make the canvas resize
  mCanvas->hide();

  // make testing layers
  mLayerLineZ = new QgsVectorLayer( QStringLiteral( "LineStringZ?crs=EPSG:3946" ), QStringLiteral( "layer line Z" ), QStringLiteral( "memory" ) );
  QVERIFY( mLayerLineZ->isValid() );
  QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayerLineZ );

  mLayerPointZ = new QgsVectorLayer( QStringLiteral( "PointZ?crs=EPSG:3946" ), QStringLiteral( "point Z" ), QStringLiteral( "memory" ) );
  QVERIFY( mLayerPointZ->isValid() );
  QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayerPointZ );

  mLayerPolygonZ = new QgsVectorLayer( QStringLiteral( "PolygonZ?crs=EPSG:3946" ), QStringLiteral( "polygon Z" ), QStringLiteral( "memory" ) );
  QVERIFY( mLayerPolygonZ->isValid() );
  QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayerPolygonZ );

  mLayerTopo = new QgsVectorLayer( QStringLiteral( "Polygon?crs=EPSG:3946" ), QStringLiteral( "topo" ), QStringLiteral( "memory" ) );
  QVERIFY( mLayerTopo->isValid() );

  mLayerTopo2 = new QgsVectorLayer( QStringLiteral( "Polygon?crs=EPSG:3946" ), QStringLiteral( "topo2" ), QStringLiteral( "memory" ) );
  QVERIFY( mLayerTopo2->isValid() );

  mLayerLine = new QgsVectorLayer( QStringLiteral( "LineString?crs=EPSG:3946" ), QStringLiteral( "layer line" ), QStringLiteral( "memory" ) );
  QVERIFY( mLayerLine->isValid() );
  QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayerLine );

  mLayerLineZ->startEditing();
  const QString wkt1 = "LineString Z (0 0 0, 1 1 0, 1 2 0)";
  const QString wkt2 = "LineString Z (2 1 5, 3 3 5)";
  QgsFeature f1;
  f1.setGeometry( QgsGeometry::fromWkt( wkt1 ) );
  QgsFeature f2;
  f2.setGeometry( QgsGeometry::fromWkt( wkt2 ) );

  QgsFeatureList flist;
  flist << f1 << f2;
  mLayerLineZ->dataProvider()->addFeatures( flist );
  QCOMPARE( mLayerLineZ->featureCount(), ( long ) 2 );
  QCOMPARE( mLayerLineZ->getFeature( 1 ).geometry().asWkt(), wkt1 );
  QCOMPARE( mLayerLineZ->getFeature( 2 ).geometry().asWkt(), wkt2 );

  mLayerPointZ->startEditing();
  const QString wkt3 = "Point Z (5 5 5)";
  QgsFeature f3;
  f3.setGeometry( QgsGeometry::fromWkt( wkt3 ) );
  const QString wkt4 = "Point Z (6 6 6)";
  QgsFeature f4;
  f4.setGeometry( QgsGeometry::fromWkt( wkt4 ) );

  QgsFeatureList flistPoint;
  flistPoint << f3 << f4;
  mLayerPointZ->dataProvider()->addFeatures( flistPoint );
  QCOMPARE( mLayerPointZ->featureCount(), ( long ) 2 );
  QCOMPARE( mLayerPointZ->getFeature( 1 ).geometry().asWkt(), wkt3 );
  QCOMPARE( mLayerPointZ->getFeature( 2 ).geometry().asWkt(), wkt4 );

  mLayerPolygonZ->startEditing();
  const QString wkt5 = "Polygon Z ((7 5 4, 3 2 1, 0 1 2, 7 5 4))";
  QgsFeature f5;
  f5.setGeometry( QgsGeometry::fromWkt( wkt5 ) );
  QgsFeatureList flistPolygon;
  flistPolygon << f5;
  mLayerPolygonZ->dataProvider()->addFeatures( flistPolygon );
  QCOMPARE( mLayerPolygonZ->featureCount(), ( long ) 1 );
  QCOMPARE( mLayerPolygonZ->getFeature( 1 ).geometry().asWkt(), wkt5 );

  mLayerTopo->startEditing();
  const QString wkt6 = "Polygon ((0 0, 4 0, 4 4, 0 4, 0 0))";
  QgsFeature f6;
  f6.setGeometry( QgsGeometry::fromWkt( wkt6 ) );
  const QString wkt7 = "Polygon ((7 0, 8 0, 8 4, 7 4, 7 0))";
  QgsFeature f7;
  f7.setGeometry( QgsGeometry::fromWkt( wkt7 ) );
  QgsFeatureList flistTopo;
  flistTopo << f6 << f7;
  mLayerTopo->dataProvider()->addFeatures( flistTopo );
  QCOMPARE( mLayerTopo->featureCount(), ( long ) 2 );
  QCOMPARE( mLayerTopo->getFeature( 1 ).geometry().asWkt(), wkt6 );
  QCOMPARE( mLayerTopo->getFeature( 2 ).geometry().asWkt(), wkt7 );

  mLayerTopo2->startEditing();
  QgsFeature f8;
  f8.setGeometry( QgsGeometry::fromWkt( QStringLiteral( "Polygon ((0 5, 4 5, 4 7, 0 7, 0 5))" ) ) );
  mLayerTopo2->dataProvider()->addFeatures( QgsFeatureList() << f8 );
  QCOMPARE( mLayerTopo2->featureCount(), 1 );
  QCOMPARE( mLayerTopo2->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 5, 4 5, 4 7, 0 7, 0 5))" ) );

  mLayerLine->startEditing();
  const QString wkt9 = QStringLiteral( "LineString (0 0, 10 10)" );
  const QString wkt10 = QStringLiteral( "LineString (2 8, 0 6)" );
  const QString wkt11 = QStringLiteral( "LineString (13 0, 13 8, 19 11, 25 8, 25 0, 13 0)" ); // cw oriented linestring ring
  QgsFeature f9, f10, f11;
  f9.setGeometry( QgsGeometry::fromWkt( wkt9 ) );
  f10.setGeometry( QgsGeometry::fromWkt( wkt10 ) );
  f11.setGeometry( QgsGeometry::fromWkt( wkt11 ) );
  mLayerLine->dataProvider()->addFeatures( QgsFeatureList() << f9 << f10 << f11 );
  QCOMPARE( mLayerLine->featureCount(), ( long ) 3 );
  QCOMPARE( mLayerLine->getFeature( 1 ).geometry().asWkt(), wkt9 );
  QCOMPARE( mLayerLine->getFeature( 2 ).geometry().asWkt(), wkt10 );
  QCOMPARE( mLayerLine->getFeature( 3 ).geometry().asWkt(), wkt11 );

  QgsSnappingConfig cfg = mCanvas->snappingUtils()->config();
  cfg.setMode( Qgis::SnappingMode::AllLayers );
  cfg.setTolerance( 100 );
  cfg.setTypeFlag( static_cast<Qgis::SnappingTypes>( Qgis::SnappingType::Vertex | Qgis::SnappingType::Segment ) );
  cfg.setEnabled( true );
  mCanvas->snappingUtils()->setConfig( cfg );
  mCanvas->setLayers( QList<QgsMapLayer *>() << mLayerLineZ << mLayerPointZ << mLayerPolygonZ );
  mCanvas->setCurrentLayer( mLayerLineZ );

  mCanvas->snappingUtils()->locatorForLayer( mLayerLineZ )->init();
  mCanvas->snappingUtils()->locatorForLayer( mLayerPointZ )->init();
  mCanvas->snappingUtils()->locatorForLayer( mLayerPolygonZ )->init();
  mCanvas->snappingUtils()->locatorForLayer( mLayerTopo )->init();
  mCanvas->snappingUtils()->locatorForLayer( mLayerTopo2 )->init();

  // create the tool
  mCaptureTool = new QgsMapToolReshape( mCanvas );
  mCanvas->setMapTool( mCaptureTool );

  QCOMPARE( mCanvas->mapSettings().outputSize(), QSize( 512, 512 ) );
  QCOMPARE( mCanvas->mapSettings().visibleExtent(), QgsRectangle( 0, 0, 8, 8 ) );
}

//runs after all tests
void TestQgsMapToolReshape::cleanupTestCase()
{
  delete mCaptureTool;
  delete mCanvas;
  QgsApplication::exitQgis();
}

void TestQgsMapToolReshape::testReshapeNotEnoughPoints()
{
  TestQgsMapToolAdvancedDigitizingUtils utils( mCaptureTool );
  // no snapping for this test
  QgsSnappingConfig cfg = mCanvas->snappingUtils()->config();
  cfg.setEnabled( false );
  mCanvas->snappingUtils()->setConfig( cfg );

  const QSignalSpy editCommandSpy( mLayerLineZ, &QgsVectorLayer::editCommandStarted );

  utils.mouseClick( 2, 2, Qt::LeftButton );
  utils.mouseClick( 3, 2, Qt::RightButton );

  // activate back snapping
  cfg.setEnabled( true );
  mCanvas->snappingUtils()->setConfig( cfg );

  QCOMPARE( editCommandSpy.count(), 0 );
  QCOMPARE( mLayerLineZ->undoStack()->index(), 0 );
}

void TestQgsMapToolReshape::testReshapeNoChange()
{
  TestQgsMapToolAdvancedDigitizingUtils utils( mCaptureTool );
  // no snapping for this test
  QgsSnappingConfig cfg = mCanvas->snappingUtils()->config();
  cfg.setEnabled( false );
  mCanvas->snappingUtils()->setConfig( cfg );

  utils.mouseClick( 2, 2, Qt::LeftButton );
  utils.mouseClick( 3, 2, Qt::LeftButton );
  utils.mouseClick( 3, 2, Qt::RightButton );

  // activate back snapping
  cfg.setEnabled( true );
  mCanvas->snappingUtils()->setConfig( cfg );

  QCOMPARE( mLayerLineZ->undoStack()->index(), 0 );
}

void TestQgsMapToolReshape::testReshapeZ()
{
  TestQgsMapToolAdvancedDigitizingUtils utils( mCaptureTool );

  // test with default Z value = 333
  QgsSettingsRegistryCore::settingsDigitizingDefaultZValue->setValue( 333 );

  // snap on a linestringz layer
  utils.mouseClick( 1, 2, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 2, 1, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 2, 1, Qt::RightButton );

  const QString wkt = "LineString Z (0 0 0, 1 1 0, 1 2 0, 2 1 5)";
  QCOMPARE( mLayerLineZ->getFeature( 1 ).geometry().asWkt(), wkt );

  // snap on a pointz layer
  utils.mouseClick( 2, 1, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 5, 5, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 6, 6, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 6, 6, Qt::RightButton );

  const QString wkt2 = "LineString Z (0 0 0, 1 1 0, 1 2 0, 2 1 5, 5 5 5, 6 6 6)";
  QCOMPARE( mLayerLineZ->getFeature( 1 ).geometry().asWkt(), wkt2 );

  // snap on a polygonz layer
  utils.mouseClick( 6, 6, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 7, 5, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 3, 2, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 3, 2, Qt::RightButton );

  const QString wkt3 = "LineString Z (0 0 0, 1 1 0, 1 2 0, 2 1 5, 5 5 5, 6 6 6, 7 5 4, 3 2 1)";
  QCOMPARE( mLayerLineZ->getFeature( 1 ).geometry().asWkt(), wkt3 );
  mLayerLineZ->undoStack()->undo();
}

void TestQgsMapToolReshape::testTopologicalEditing()
{
  QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayerTopo );
  mCanvas->setLayers( QList<QgsMapLayer *>() << mLayerTopo );

  const bool topologicalEditing = QgsProject::instance()->topologicalEditing();
  QgsProject::instance()->setTopologicalEditing( true );
  mCanvas->setCurrentLayer( mLayerTopo );
  TestQgsMapToolAdvancedDigitizingUtils utils( mCaptureTool );

  // test with default Z value = 333
  QgsSettingsRegistryCore::settingsDigitizingDefaultZValue->setValue( 333 );

  utils.mouseClick( 4, 4, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 8, 2, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 4, 0, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 4, 0, Qt::RightButton );

  const QString wkt = "Polygon ((4 0, 8 2, 4 4, 0 4, 0 0, 4 0))";
  const QString wkt2 = "Polygon ((7 0, 8 0, 8 2, 8 4, 7 4, 7 0))";

  QCOMPARE( mLayerTopo->getFeature( 1 ).geometry().asWkt(), wkt );
  QCOMPARE( mLayerTopo->getFeature( 2 ).geometry().asWkt(), wkt2 );

  mLayerTopo->undoStack()->undo();

  QCOMPARE( mLayerTopo2->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 5, 4 5, 4 7, 0 7, 0 5))" ) );
  QCOMPARE( mLayerTopo->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 0, 4 0, 4 4, 0 4, 0 0))" ) );
  QCOMPARE( mLayerTopo->getFeature( 2 ).geometry().asWkt(), QStringLiteral( "Polygon ((7 0, 8 0, 8 4, 7 4, 7 0))" ) );

  QgsProject::instance()->setTopologicalEditing( topologicalEditing );
}

void TestQgsMapToolReshape::testTopologicalEditingNoSnap()
{
  QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayerTopo );
  mCanvas->setLayers( QList<QgsMapLayer *>() << mLayerTopo );

  QgsSnappingConfig cfg = mCanvas->snappingUtils()->config();
  cfg.setEnabled( false );
  mCanvas->snappingUtils()->setConfig( cfg );

  const bool topologicalEditing = QgsProject::instance()->topologicalEditing();
  QgsProject::instance()->setTopologicalEditing( true );
  mCanvas->setCurrentLayer( mLayerTopo );
  TestQgsMapToolAdvancedDigitizingUtils utils( mCaptureTool );

  utils.mouseClick( 4, 4, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 8, 2, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 4, 0, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 4, 0, Qt::RightButton );

  const QString wkt = "Polygon ((4 0, 8 2, 4 4, 0 4, 0 0, 4 0))";
  const QString wkt2 = "Polygon ((7 0, 8 0, 8 2, 8 4, 7 4, 7 0))";

  QCOMPARE( mLayerTopo->getFeature( 1 ).geometry().asWkt(), wkt );
  QCOMPARE( mLayerTopo->getFeature( 2 ).geometry().asWkt(), wkt2 );

  mLayerTopo->undoStack()->undo();

  QCOMPARE( mLayerTopo2->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 5, 4 5, 4 7, 0 7, 0 5))" ) );
  QCOMPARE( mLayerTopo->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 0, 4 0, 4 4, 0 4, 0 0))" ) );
  QCOMPARE( mLayerTopo->getFeature( 2 ).geometry().asWkt(), QStringLiteral( "Polygon ((7 0, 8 0, 8 4, 7 4, 7 0))" ) );

  QgsProject::instance()->setTopologicalEditing( topologicalEditing );
  cfg.setEnabled( true );
  mCanvas->snappingUtils()->setConfig( cfg );
}

void TestQgsMapToolReshape::testAvoidIntersectionAndTopoEdit()
{
  QList<QgsMapLayer *> layers = { mLayerTopo, mLayerTopo2 };
  QgsProject::instance()->addMapLayers( layers );
  mCanvas->setLayers( layers );

  // backup project settings
  const bool topologicalEditing = QgsProject::instance()->topologicalEditing();
  const Qgis::AvoidIntersectionsMode mode( QgsProject::instance()->avoidIntersectionsMode() );
  const QList<QgsVectorLayer *> vlayers = QgsProject::instance()->avoidIntersectionsLayers();
  const bool isAutoSnapEnabled = mCaptureTool->isAutoSnapEnabled();

  QgsProject::instance()->setAvoidIntersectionsMode( Qgis::AvoidIntersectionsMode::AvoidIntersectionsLayers );
  QgsProject::instance()->setAvoidIntersectionsLayers( { mLayerTopo, mLayerTopo2 } );
  QgsProject::instance()->setTopologicalEditing( true );
  mCanvas->setCurrentLayer( mLayerTopo2 );
  mCaptureTool->setAutoSnapEnabled( false );
  TestQgsMapToolAdvancedDigitizingUtils utils( mCaptureTool );

  // reshape mLayerTopo2 feature 1 with a point inside mLayerTopo feature 1, there is 2 topo point added on this last
  utils.mouseClick( 0, 5, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 2, 3, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 4, 5, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 4, 5, Qt::RightButton );

  QCOMPARE( mLayerTopo2->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 5, 0 7, 4 7, 4 5, 3 4, 1 4, 0 5))" ) );
  QCOMPARE( mLayerTopo->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 0, 4 0, 4 4, 3 4, 1 4, 0 4, 0 0))" ) );
  QCOMPARE( mLayerTopo->getFeature( 2 ).geometry().asWkt(), QStringLiteral( "Polygon ((7 0, 8 0, 8 4, 7 4, 7 0))" ) );

  mLayerTopo2->undoStack()->undo();
  mLayerTopo->undoStack()->undo();

  QCOMPARE( mLayerTopo2->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 5, 4 5, 4 7, 0 7, 0 5))" ) );
  QCOMPARE( mLayerTopo->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 0, 4 0, 4 4, 0 4, 0 0))" ) );
  QCOMPARE( mLayerTopo->getFeature( 2 ).geometry().asWkt(), QStringLiteral( "Polygon ((7 0, 8 0, 8 4, 7 4, 7 0))" ) );

  QgsProject::instance()->setTopologicalEditing( topologicalEditing );
  QgsProject::instance()->setAvoidIntersectionsMode( mode );
  QgsProject::instance()->setAvoidIntersectionsLayers( vlayers );
  mCaptureTool->setAutoSnapEnabled( isAutoSnapEnabled );
}

void TestQgsMapToolReshape::testAvoidIntersectionAndTopoEditSameLayer()
{
  QList<QgsMapLayer *> layers = { mLayerTopo, mLayerTopo2 };
  QgsProject::instance()->addMapLayers( layers );
  mCanvas->setLayers( layers );

  // backup project settings
  const bool topologicalEditing = QgsProject::instance()->topologicalEditing();
  const Qgis::AvoidIntersectionsMode mode( QgsProject::instance()->avoidIntersectionsMode() );
  const QList<QgsVectorLayer *> vlayers = QgsProject::instance()->avoidIntersectionsLayers();
  const bool isAutoSnapEnabled = mCaptureTool->isAutoSnapEnabled();

  QgsProject::instance()->setAvoidIntersectionsMode( Qgis::AvoidIntersectionsMode::AvoidIntersectionsLayers );
  QgsProject::instance()->setAvoidIntersectionsLayers( { mLayerTopo, mLayerTopo2 } );
  QgsProject::instance()->setTopologicalEditing( true );
  mCanvas->setCurrentLayer( mLayerTopo );
  mCaptureTool->setAutoSnapEnabled( false );
  TestQgsMapToolAdvancedDigitizingUtils utils( mCaptureTool );

  // reshape mLayerTopo feature 1 with two points inside mLayerTopo feature 2, both features should be reshaped
  utils.mouseClick( 4, 4, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 7, 4, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 7.5, 3, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 7.5, 1, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 7, 0, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 4, 0, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 4, 0, Qt::RightButton );

  QCOMPARE( mLayerTopo2->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 5, 4 5, 4 7, 0 7, 0 5))" ) );
  QCOMPARE( mLayerTopo->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((4 0, 7 0, 7.5 1, 7.5 3, 7 4, 4 4, 0 4, 0 0, 4 0))" ) );
  QCOMPARE( mLayerTopo->getFeature( 2 ).geometry().asWkt(), QStringLiteral( "Polygon ((7 0, 8 0, 8 4, 7 4, 7.5 3, 7.5 1, 7 0))" ) );

  mLayerTopo->undoStack()->undo();

  QCOMPARE( mLayerTopo2->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 5, 4 5, 4 7, 0 7, 0 5))" ) );
  QCOMPARE( mLayerTopo->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 0, 4 0, 4 4, 0 4, 0 0))" ) );
  QCOMPARE( mLayerTopo->getFeature( 2 ).geometry().asWkt(), QStringLiteral( "Polygon ((7 0, 8 0, 8 4, 7 4, 7 0))" ) );

  QgsProject::instance()->setTopologicalEditing( topologicalEditing );
  QgsProject::instance()->setAvoidIntersectionsMode( mode );
  QgsProject::instance()->setAvoidIntersectionsLayers( vlayers );
  mCaptureTool->setAutoSnapEnabled( isAutoSnapEnabled );
}

void TestQgsMapToolReshape::testAvoidIntersectionAndTopoEditSameLayerSelection()
{
  QList<QgsMapLayer *> layers = { mLayerTopo, mLayerTopo2 };
  QgsProject::instance()->addMapLayers( layers );
  mCanvas->setLayers( layers );

  // backup project settings
  const bool topologicalEditing = QgsProject::instance()->topologicalEditing();
  const Qgis::AvoidIntersectionsMode mode( QgsProject::instance()->avoidIntersectionsMode() );
  const QList<QgsVectorLayer *> vlayers = QgsProject::instance()->avoidIntersectionsLayers();
  const bool isAutoSnapEnabled = mCaptureTool->isAutoSnapEnabled();

  QgsProject::instance()->setAvoidIntersectionsMode( Qgis::AvoidIntersectionsMode::AvoidIntersectionsLayers );
  QgsProject::instance()->setAvoidIntersectionsLayers( { mLayerTopo, mLayerTopo2 } );
  QgsProject::instance()->setTopologicalEditing( true );
  mCanvas->setCurrentLayer( mLayerTopo );
  mCaptureTool->setAutoSnapEnabled( false );
  TestQgsMapToolAdvancedDigitizingUtils utils( mCaptureTool );

  mLayerTopo->selectByIds( { 1 } );

  // reshape mLayerTopo feature 1 with two points inside mLayerTopo feature 2, only the selected feature should be reshaped
  utils.mouseClick( 4, 4, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 7, 4, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 7.5, 3, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 7.5, 1, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 7, 0, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 4, 0, Qt::LeftButton, Qt::KeyboardModifiers() );
  utils.mouseClick( 4, 0, Qt::RightButton );

  QCOMPARE( mLayerTopo2->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 5, 4 5, 4 7, 0 7, 0 5))" ) );
  QCOMPARE( mLayerTopo->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((4 0, 0 0, 0 4, 4 4, 7 4, 7 0, 4 0))" ) );
  QCOMPARE( mLayerTopo->getFeature( 2 ).geometry().asWkt(), QStringLiteral( "Polygon ((7 0, 8 0, 8 4, 7 4, 7 0))" ) );

  mLayerTopo->undoStack()->undo();

  QCOMPARE( mLayerTopo2->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 5, 4 5, 4 7, 0 7, 0 5))" ) );
  QCOMPARE( mLayerTopo->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 0, 4 0, 4 4, 0 4, 0 0))" ) );
  QCOMPARE( mLayerTopo->getFeature( 2 ).geometry().asWkt(), QStringLiteral( "Polygon ((7 0, 8 0, 8 4, 7 4, 7 0))" ) );

  mLayerTopo->removeSelection();
  QgsProject::instance()->setTopologicalEditing( topologicalEditing );
  QgsProject::instance()->setAvoidIntersectionsMode( mode );
  QgsProject::instance()->setAvoidIntersectionsLayers( vlayers );
  mCaptureTool->setAutoSnapEnabled( isAutoSnapEnabled );
}

void TestQgsMapToolReshape::reshapeWithBindingLine()
{
  // prepare vector layer
  std::unique_ptr<QgsVectorLayer> vl;
  vl.reset( new QgsVectorLayer( QStringLiteral( "LineString?crs=epsg:4326&field=name:string(20)" ), QStringLiteral( "vl" ), QStringLiteral( "memory" ) ) );

  const QgsGeometry g0 = QgsGeometry::fromWkt( "LineString (0 0, 1 1, 1 2)" );
  QgsFeature f0;
  f0.setGeometry( g0 );
  f0.setAttribute( 0, "polyline0" );

  const QgsGeometry g1 = QgsGeometry::fromWkt( "LineString (2 1, 3 2, 3 3, 2 2)" );
  QgsFeature f1;
  f1.setGeometry( g1 );
  f1.setAttribute( 0, "polyline1" );

  vl->dataProvider()->addFeatures( QgsFeatureList() << f0 << f1 );

  // prepare canvas
  QList<QgsMapLayer *> layers;
  layers.append( vl.get() );

  const QgsCoordinateReferenceSystem srs( QStringLiteral( "EPSG:4326" ) );
  mQgisApp->mapCanvas()->setDestinationCrs( srs );
  mQgisApp->mapCanvas()->setLayers( layers );
  mQgisApp->mapCanvas()->setCurrentLayer( vl.get() );

  // reshape to add line to polyline0
  QgsLineString cl0;
  cl0.setPoints( QgsPointSequence() << QgsPoint( 1, 2 ) << QgsPoint( 2, 1 ) );

  const QgsCompoundCurve curve0( *cl0.toCurveType() );

  QgsMapToolReshape tool0( mQgisApp->mapCanvas() );
  tool0.addCurve( curve0.clone() );

  vl->startEditing();
  tool0.reshape( vl.get() );

  f0 = vl->getFeature( 1 );
  QCOMPARE( f0.geometry().asWkt(), QStringLiteral( "LineString (0 0, 1 1, 1 2, 2 1)" ) );

  f1 = vl->getFeature( 2 );
  QCOMPARE( f1.geometry().asWkt(), QStringLiteral( "LineString (2 1, 3 2, 3 3, 2 2)" ) );

  vl->rollBack();

  // reshape to add line to polyline1
  QgsLineString cl1;
  cl1.setPoints( QgsPointSequence() << QgsPoint( 2, 1 ) << QgsPoint( 1, 2 ) );

  const QgsCompoundCurve curve1( *cl1.toCurveType() );

  QgsMapToolReshape tool1( mQgisApp->mapCanvas() );
  tool1.addCurve( curve1.clone() );

  vl->startEditing();
  tool1.reshape( vl.get() );

  f0 = vl->getFeature( 1 );
  QCOMPARE( f0.geometry().asWkt(), QStringLiteral( "LineString (0 0, 1 1, 1 2)" ) );

  f1 = vl->getFeature( 2 );
  QCOMPARE( f1.geometry().asWkt(), QStringLiteral( "LineString (1 2, 2 1, 3 2, 3 3, 2 2)" ) );

  vl->rollBack();
}

void TestQgsMapToolReshape::testWithTracing()
{
  QgsProject::instance()->addMapLayers( QList<QgsMapLayer *>() << mLayerTopo );
  mCanvas->setLayers( QList<QgsMapLayer *>() << mLayerTopo );

  auto tracer = std::make_unique<QgsMapCanvasTracer>( mCanvas, nullptr );

  const bool topologicalEditing = QgsProject::instance()->topologicalEditing();
  QgsProject::instance()->setTopologicalEditing( true );
  mCanvas->setCurrentLayer( mLayerTopo );
  TestQgsMapToolAdvancedDigitizingUtils utils( mCaptureTool );

  // test with default Z value = 333
  QgsSettingsRegistryCore::settingsDigitizingDefaultZValue->setValue( 333 );

  utils.mouseClick( 7, 0, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 4, 0, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 4, 4, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 7, 4, Qt::LeftButton, Qt::KeyboardModifiers(), true );
  utils.mouseClick( 8, 5, Qt::RightButton );

  QCOMPARE( mLayerTopo->getFeature( 1 ).geometry().asWkt(), "Polygon ((0 0, 4 0, 4 4, 0 4, 0 0))" );
  QCOMPARE( mLayerTopo->getFeature( 2 ).geometry().asWkt(), "Polygon ((7 0, 8 0, 8 4, 7 4, 4 4, 4 0, 7 0))" );

  mLayerTopo->undoStack()->undo();

  QCOMPARE( mLayerTopo->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon ((0 0, 4 0, 4 4, 0 4, 0 0))" ) );
  QCOMPARE( mLayerTopo->getFeature( 2 ).geometry().asWkt(), QStringLiteral( "Polygon ((7 0, 8 0, 8 4, 7 4, 7 0))" ) );

  QgsProject::instance()->setTopologicalEditing( topologicalEditing );
}

void TestQgsMapToolReshape::testKeepDirection()
{
  TestQgsMapToolAdvancedDigitizingUtils utils( mCaptureTool );
  mCanvas->setCurrentLayer( mLayerLine );

  // no snapping for this test
  QgsSnappingConfig cfg = mCanvas->snappingUtils()->config();
  cfg.setEnabled( false );
  mCanvas->snappingUtils()->setConfig( cfg );

  // intersect linestring 4 times, and go backwards compared to its direction
  utils.mouseClick( 9, 8, Qt::LeftButton );
  utils.mouseClick( 9, 10, Qt::LeftButton );
  utils.mouseClick( 8, 9, Qt::LeftButton );
  utils.mouseClick( 8, 7, Qt::LeftButton );
  utils.mouseClick( 7, 6, Qt::LeftButton );
  utils.mouseClick( 7, 8, Qt::LeftButton );
  utils.mouseClick( 6, 7, Qt::LeftButton );
  utils.mouseClick( 6, 5, Qt::LeftButton );
  utils.mouseClick( 6, 5, Qt::RightButton );

  QString wkt1 = QStringLiteral( "LineString (0 0, 6 6, 6 7, 7 8, 7 7, 7 6, 8 7, 8 8, 8 9, 9 10, 9 9, 10 10)" );
  QCOMPARE( mLayerLine->getFeature( 1 ).geometry().asWkt(), wkt1 );

  // extend linestring from its start point
  utils.mouseClick( 2, 8, Qt::LeftButton );
  utils.mouseClick( 3, 9, Qt::LeftButton );
  utils.mouseClick( 3, 9, Qt::RightButton );

  QString wkt2 = QStringLiteral( "LineString (3 9, 2 8, 0 6)" );
  QCOMPARE( mLayerLine->getFeature( 2 ).geometry().asWkt(), wkt2 );

  // intersect linestring ring 4 times, and go backwards compared to its direction
  utils.mouseClick( 14, 7, Qt::LeftButton );
  utils.mouseClick( 12, 7, Qt::LeftButton );
  utils.mouseClick( 12, 5, Qt::LeftButton );
  utils.mouseClick( 14, 5, Qt::LeftButton );
  utils.mouseClick( 14, 3, Qt::LeftButton );
  utils.mouseClick( 12, 3, Qt::LeftButton );
  utils.mouseClick( 12, 1, Qt::LeftButton );
  utils.mouseClick( 14, 1, Qt::LeftButton );
  utils.mouseClick( 14, 1, Qt::RightButton );

  QString wkt3 = QStringLiteral( "LineString (13 1, 12 1, 12 3, 13 3, 14 3, 14 5, 13 5, 12 5, 12 7, 13 7, 13 8, 19 11, 25 8, 25 0, 13 0, 13 1)" );
  QCOMPARE( mLayerLine->getFeature( 3 ).geometry().asWkt(), wkt3 );

  // undo the three changes
  mLayerLine->undoStack()->undo();
  mLayerLine->undoStack()->undo();
  mLayerLine->undoStack()->undo();

  // activate back snapping
  cfg.setEnabled( true );
  mCanvas->snappingUtils()->setConfig( cfg );
}

void TestQgsMapToolReshape::testWithSnapToSegment()
{
  TestQgsMapToolAdvancedDigitizingUtils utils( mCaptureTool );
  mCanvas->setLayers( { mLayerPolygonZ } );
  mCanvas->setCurrentLayer( mLayerPolygonZ );
  mCanvas->setDestinationCrs( mLayerPolygonZ->crs() );

  QgsSnappingConfig cfg = mCanvas->snappingUtils()->config();
  cfg.setTypeFlag( static_cast<Qgis::SnappingTypes>( Qgis::SnappingType::Segment ) );
  mCanvas->snappingUtils()->setConfig( cfg );

  QCOMPARE( mLayerPolygonZ->getFeature( 1 ).geometry().asWkt(), QStringLiteral( "Polygon Z ((7 5 4, 3 2 1, 0 1 2, 7 5 4))" ) );

  // snap to segment on a diagonal
  utils.mouseClick( 5.5, 4.5, Qt::LeftButton, {}, true );
  utils.mouseClick( 1, 5, Qt::LeftButton );
  utils.mouseClick( 1, 2, Qt::LeftButton, {}, true );
  utils.mouseClick( 1, 2, Qt::RightButton );

  QCOMPARE( mLayerPolygonZ->getFeature( 1 ).geometry().asWkt( 1 ), QStringLiteral( "Polygon Z ((1.2 1.7 333, 1 5 333, 5.7 4.2 333, 7 5 4, 3 2 1, 0 1 2, 1.2 1.7 333))" ) );

  mLayerLine->undoStack()->undo();

  cfg.setTypeFlag( static_cast<Qgis::SnappingTypes>( Qgis::SnappingType::Vertex | Qgis::SnappingType::Segment ) );
  mCanvas->snappingUtils()->setConfig( cfg );
}

QGSTEST_MAIN( TestQgsMapToolReshape )
#include "testqgsmaptoolreshape.moc"
