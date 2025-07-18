/***************************************************************************
                             qgslayoutapputils.cpp
                             ---------------------
    Date                 : October 2017
    Copyright            : (C) 2017 Nyall Dawson
    Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgslayoutguiutils.h"
#include "qgsgui.h"
#include "qgslayoutitemguiregistry.h"
#include "qgslayoutitemregistry.h"
#include "qgslayoutviewrubberband.h"
#include "qgslayoutitemshape.h"
#include "qgslayoutmapwidget.h"
#include "qgslayoutshapewidget.h"
#include "qgslayoutmarkerwidget.h"
#include "qgslayoutitemmap.h"
#include "qgslayoutitempolygon.h"
#include "qgslayoutitempolyline.h"
#include "qgslayoutitemmarker.h"
#include "qgslayoutpolygonwidget.h"
#include "qgslayoutpolylinewidget.h"
#include "qgslayoutpicturewidget.h"
#include "qgslayoutitempicture.h"
#include "qgslayoutitemlabel.h"
#include "qgslayoutitemelevationprofile.h"
#include "qgslayoutlabelwidget.h"
#include "qgslayoutitemlegend.h"
#include "qgslayoutitemscalebar.h"
#include "qgslayoutlegendwidget.h"
#include "qgslayoutframe.h"
#include "qgslayoutitemhtml.h"
#include "qgslayouthtmlwidget.h"
#include "qgslayoutscalebarwidget.h"
#include "qgslayoutitemattributetable.h"
#include "qgslayoutattributetablewidget.h"
#include "qgslayoutitemmanualtable.h"
#include "qgslayoutmanualtablewidget.h"
#include "qgslayoutelevationprofilewidget.h"
#include "qgsmapcanvas.h"
#include "qgsplot.h"
#include "qgsfontutils.h"

/**
 * Attempts to find the best guess at a map item to link \a referenceItem to,
 * by:
 *
 * - Prioritizing a selected map
 * - If no selection, prioritizing the topmost map the item was drawn over
 * - If still none, use the layout's reference map (or biggest map)
 */
QgsLayoutItemMap *findSensibleDefaultLinkedMapItem( QgsLayoutItem *referenceItem )
{
  // start by trying to find a selected map
  QList<QgsLayoutItemMap *> mapItems;
  referenceItem->layout()->layoutItems( mapItems );

  QgsLayoutItemMap *targetMap = nullptr;
  for ( QgsLayoutItemMap *map : std::as_const( mapItems ) )
  {
    if ( map->isSelected() )
    {
      return map;
    }
  }

  // nope, no selection... hm, was the item drawn over a map? If so, use the topmost intersecting one
  double largestZValue = std::numeric_limits<double>::lowest();
  for ( QgsLayoutItemMap *map : std::as_const( mapItems ) )
  {
    if ( map->collidesWithItem( referenceItem ) && map->zValue() > largestZValue )
    {
      targetMap = map;
      largestZValue = map->zValue();
    }
  }
  if ( targetMap )
    return targetMap;

  // ah frick it, just use the reference (or biggest!) map
  return referenceItem->layout()->referenceMap();
}

void QgsLayoutGuiUtils::registerGuiForKnownItemTypes( QgsMapCanvas *mapCanvas )
{
  QgsLayoutItemGuiRegistry *registry = QgsGui::layoutItemGuiRegistry();

  registry->addItemGroup( QgsLayoutItemGuiGroup( QStringLiteral( "shapes" ), QObject::tr( "Shape" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddBasicShape.svg" ) ) ) );
  registry->addItemGroup( QgsLayoutItemGuiGroup( QStringLiteral( "nodes" ), QObject::tr( "Node Item" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddNodesItem.svg" ) ) ) );

  auto createRubberBand = ( []( QgsLayoutView *view ) -> QgsLayoutViewRubberBand * {
    return new QgsLayoutViewRectangularRubberBand( view );
  } );
  auto createEllipseBand = ( []( QgsLayoutView *view ) -> QgsLayoutViewRubberBand * {
    return new QgsLayoutViewEllipticalRubberBand( view );
  } );
  auto createTriangleBand = ( []( QgsLayoutView *view ) -> QgsLayoutViewRubberBand * {
    return new QgsLayoutViewTriangleRubberBand( view );
  } );

#if 0
  registry->addLayoutItemGuiMetadata( new QgsLayoutItemGuiMetadata( QgsLayoutItemRegistry::LayoutItem + 1002, QStringLiteral( "test" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddLabel.svg" ) ), nullptr, createRubberBand ) );
#endif

  // map item

  auto mapItemMetadata = std::make_unique<QgsLayoutItemGuiMetadata>( QgsLayoutItemRegistry::LayoutMap, QObject::tr( "Map" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddMap.svg" ) ), [mapCanvas]( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * { return new QgsLayoutMapWidget( qobject_cast<QgsLayoutItemMap *>( item ), mapCanvas ); }, createRubberBand );
  mapItemMetadata->setItemAddedToLayoutFunction( [mapCanvas]( QgsLayoutItem *item, const QVariantMap & ) {
    QgsLayoutItemMap *map = qobject_cast<QgsLayoutItemMap *>( item );
    Q_ASSERT( map );

    //get the color for map canvas background and set map background color accordingly
    map->setBackgroundColor( QgsProject::instance()->backgroundColor() );

    if ( mapCanvas )
    {
      map->setMapRotation( mapCanvas->rotation() );
      map->zoomToExtent( mapCanvas->mapSettings().visibleExtent() );
    }

    // auto assign a unique id to map items
    QList<QgsLayoutItemMap *> mapsList;
    if ( map->layout() )
      map->layout()->layoutItems( mapsList );

    int counter = mapsList.size() + 1;
    bool existing = false;
    while ( true )
    {
      existing = false;
      for ( QgsLayoutItemMap *otherMap : std::as_const( mapsList ) )
      {
        if ( map == otherMap )
          continue;

        if ( otherMap->id() == QObject::tr( "Map %1" ).arg( counter ) )
        {
          existing = true;
          break;
        }
      }
      if ( existing )
        counter++;
      else
        break;
    }
    map->setId( QObject::tr( "Map %1" ).arg( counter ) );
  } );
  registry->addLayoutItemGuiMetadata( mapItemMetadata.release() );

  // picture item

  registry->addLayoutItemGuiMetadata( new QgsLayoutItemGuiMetadata( QgsLayoutItemRegistry::LayoutPicture, QObject::tr( "Picture" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddImage.svg" ) ), []( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * { return new QgsLayoutPictureWidget( qobject_cast<QgsLayoutItemPicture *>( item ) ); }, createRubberBand ) );


  // label item

  auto labelItemMetadata = std::make_unique<QgsLayoutItemGuiMetadata>( QgsLayoutItemRegistry::LayoutLabel, QObject::tr( "Label" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionLabel.svg" ) ), []( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * { return new QgsLayoutLabelWidget( qobject_cast<QgsLayoutItemLabel *>( item ) ); }, createRubberBand );
  labelItemMetadata->setItemAddedToLayoutFunction( []( QgsLayoutItem *item, const QVariantMap &properties ) {
    QgsLayoutItemLabel *label = qobject_cast<QgsLayoutItemLabel *>( item );
    Q_ASSERT( label );

    label->setText( properties.value( QStringLiteral( "expression" ) ).toString().isEmpty() ? QObject::tr( "Lorem ipsum" ) : QStringLiteral( "[% %1 %]" ).arg( properties.value( QStringLiteral( "expression" ) ).toString() ) );
    if ( QApplication::isRightToLeft() )
    {
      label->setHAlign( Qt::AlignRight );
    }
    QSizeF minSize = label->sizeForText();
    QSizeF currentSize = label->rect().size();

    //make sure label size is sufficient to fit text
    double labelWidth = std::max( minSize.width(), currentSize.width() );
    double labelHeight = std::max( minSize.height(), currentSize.height() );
    label->attemptSetSceneRect( QRectF( label->pos().x(), label->pos().y(), labelWidth, labelHeight ) );
  } );

  labelItemMetadata->setItemDoubleClickedFunction( []( QgsLayoutItem *item, Qgis::MouseHandlesAction action ) {
    QgsLayoutItemLabel *label = qobject_cast<QgsLayoutItemLabel *>( item );

    // size to text doesn't have any real meaning for HTML content, skip it
    if ( label->mode() == QgsLayoutItemLabel::ModeHtml )
      return;

    Q_ASSERT( label );
    QgsLayoutItem::ReferencePoint reference = QgsLayoutItem::ReferencePoint::UpperLeft;
    switch ( action )
    {
      case Qgis::MouseHandlesAction::MoveItem:
      case Qgis::MouseHandlesAction::NoAction:
      case Qgis::MouseHandlesAction::SelectItem:
      case Qgis::MouseHandlesAction::RotateTopLeft:
      case Qgis::MouseHandlesAction::RotateTopRight:
      case Qgis::MouseHandlesAction::RotateBottomLeft:
      case Qgis::MouseHandlesAction::RotateBottomRight:
        return;

      case Qgis::MouseHandlesAction::ResizeUp:
        reference = QgsLayoutItem::ReferencePoint::LowerMiddle;
        break;

      case Qgis::MouseHandlesAction::ResizeDown:
        reference = QgsLayoutItem::ReferencePoint::UpperMiddle;
        break;

      case Qgis::MouseHandlesAction::ResizeLeft:
        reference = QgsLayoutItem::ReferencePoint::MiddleRight;
        break;

      case Qgis::MouseHandlesAction::ResizeRight:
        reference = QgsLayoutItem::ReferencePoint::MiddleLeft;
        break;

      case Qgis::MouseHandlesAction::ResizeLeftUp:
        reference = QgsLayoutItem::ReferencePoint::LowerRight;
        break;

      case Qgis::MouseHandlesAction::ResizeRightUp:
        reference = QgsLayoutItem::ReferencePoint::LowerLeft;
        break;

      case Qgis::MouseHandlesAction::ResizeLeftDown:
        reference = QgsLayoutItem::ReferencePoint::UpperRight;
        break;

      case Qgis::MouseHandlesAction::ResizeRightDown:
        reference = QgsLayoutItem::ReferencePoint::UpperLeft;
        break;
    }

    label->beginCommand( QObject::tr( "Resize to Text" ) );
    label->adjustSizeToText( reference );
    label->endCommand();
  } );

  registry->addLayoutItemGuiMetadata( labelItemMetadata.release() );


  // legend item

  auto legendItemMetadata = std::make_unique<QgsLayoutItemGuiMetadata>( QgsLayoutItemRegistry::LayoutLegend, QObject::tr( "Legend" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddLegend.svg" ) ), [mapCanvas]( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * { return new QgsLayoutLegendWidget( qobject_cast<QgsLayoutItemLegend *>( item ), mapCanvas ); }, createRubberBand );
  legendItemMetadata->setItemAddedToLayoutFunction( []( QgsLayoutItem *item, const QVariantMap & ) {
    QgsLayoutItemLegend *legend = qobject_cast<QgsLayoutItemLegend *>( item );
    Q_ASSERT( legend );

    // try to find a good map to link the legend with by default
    legend->setLinkedMap( findSensibleDefaultLinkedMapItem( legend ) );

    if ( QApplication::isRightToLeft() )
    {
      // for right-to-left locales, use an appropriate default layout
      legend->setSymbolAlignment( Qt::AlignRight );
      legend->rstyle( Qgis::LegendComponent::Group ).setAlignment( Qt::AlignRight );
      legend->rstyle( Qgis::LegendComponent::Subgroup ).setAlignment( Qt::AlignRight );
      legend->rstyle( Qgis::LegendComponent::SymbolLabel ).setAlignment( Qt::AlignRight );
      legend->setTitleAlignment( Qt::AlignRight );
    }

    //set default legend font from settings
    QgsSettings settings;
    const QString defaultFontString = settings.value( QStringLiteral( "LayoutDesigner/defaultFont" ), QVariant(), QgsSettings::Gui ).toString();
    if ( !defaultFontString.isEmpty() )
    {
      QFont font;
      QgsFontUtils::setFontFamily( font, defaultFontString );

      QgsTextFormat f = legend->rstyle( Qgis::LegendComponent::Title ).textFormat();
      f.setFont( font );
      legend->rstyle( Qgis::LegendComponent::Title ).setTextFormat( f );

      f = legend->rstyle( Qgis::LegendComponent::Group ).textFormat();
      f.setFont( font );
      legend->rstyle( Qgis::LegendComponent::Group ).setTextFormat( f );

      f = legend->rstyle( Qgis::LegendComponent::Subgroup ).textFormat();
      f.setFont( font );
      legend->rstyle( Qgis::LegendComponent::Subgroup ).setTextFormat( f );

      f = legend->rstyle( Qgis::LegendComponent::SymbolLabel ).textFormat();
      f.setFont( font );
      legend->rstyle( Qgis::LegendComponent::SymbolLabel ).setTextFormat( f );
    }

    legend->updateLegend();
  } );

  registry->addLayoutItemGuiMetadata( legendItemMetadata.release() );

  // scalebar item

  auto scalebarItemMetadata = std::make_unique<QgsLayoutItemGuiMetadata>( QgsLayoutItemRegistry::LayoutScaleBar, QObject::tr( "Scale Bar" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionScaleBar.svg" ) ), []( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * { return new QgsLayoutScaleBarWidget( qobject_cast<QgsLayoutItemScaleBar *>( item ) ); }, createRubberBand );
  scalebarItemMetadata->setItemAddedToLayoutFunction( []( QgsLayoutItem *item, const QVariantMap & ) {
    QgsLayoutItemScaleBar *scalebar = qobject_cast<QgsLayoutItemScaleBar *>( item );
    Q_ASSERT( scalebar );

    // default to project's scale calculation method
    scalebar->setMethod( scalebar->layout()->project()->scaleMethod() );

    // try to find a good map to link the scalebar with by default
    if ( QgsLayoutItemMap *targetMap = findSensibleDefaultLinkedMapItem( scalebar ) )
    {
      scalebar->setLinkedMap( targetMap );
      scalebar->applyDefaultSize( scalebar->guessUnits() );
    }
  } );

  registry->addLayoutItemGuiMetadata( scalebarItemMetadata.release() );


  // north arrow
  auto northArrowMetadata = std::make_unique<QgsLayoutItemGuiMetadata>(
    QgsLayoutItemRegistry::LayoutPicture, QObject::tr( "North Arrow" ), QgsApplication::getThemeIcon( QStringLiteral( "/north_arrow.svg" ) ),
    []( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * {
      return new QgsLayoutPictureWidget( qobject_cast<QgsLayoutItemPicture *>( item ) );
    },
    createRubberBand
  );
  northArrowMetadata->setItemCreationFunction( []( QgsLayout *layout ) -> QgsLayoutItem * {
    // count how many existing north arrows are already in layout
    QList<QgsLayoutItemPicture *> pictureItems;
    layout->layoutItems( pictureItems );
    int northArrowCount = 0;

    QgsSettings settings;
    const QString defaultPath = settings.value( QStringLiteral( "LayoutDesigner/defaultNorthArrow" ), QStringLiteral( ":/images/north_arrows/layout_default_north_arrow.svg" ), QgsSettings::Gui ).toString();

    for ( QgsLayoutItemPicture *p : std::as_const( pictureItems ) )
    {
      // look for pictures which use the default north arrow svg
      if ( p->picturePath() == defaultPath )
        northArrowCount++;
    }

    auto picture = std::make_unique<QgsLayoutItemPicture>( layout );
    picture->setNorthMode( QgsLayoutItemPicture::GridNorth );
    picture->setPicturePath( defaultPath );
    // set an id by default, so that north arrows are discernible in layout item lists
    picture->setId( northArrowCount > 0 ? QObject::tr( "North Arrow %1" ).arg( northArrowCount + 1 ) : QObject::tr( "North Arrow" ) );
    return picture.release();
  } );
  northArrowMetadata->setItemAddedToLayoutFunction( []( QgsLayoutItem *item, const QVariantMap & ) {
    QgsLayoutItemPicture *picture = qobject_cast<QgsLayoutItemPicture *>( item );
    Q_ASSERT( picture );

    QList<QgsLayoutItemMap *> mapItems;
    picture->layout()->layoutItems( mapItems );

    // try to find a good map to link the north arrow with by default
    picture->setLinkedMap( findSensibleDefaultLinkedMapItem( picture ) );
  } );
  registry->addLayoutItemGuiMetadata( northArrowMetadata.release() );

  // shape items

  auto createShapeWidget =
    []( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * {
    return new QgsLayoutShapeWidget( qobject_cast<QgsLayoutItemShape *>( item ) );
  };

  registry->addLayoutItemGuiMetadata( new QgsLayoutItemGuiMetadata( QgsLayoutItemRegistry::LayoutShape, QObject::tr( "Rectangle" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddBasicRectangle.svg" ) ), createShapeWidget, createRubberBand, QStringLiteral( "shapes" ), false, QgsLayoutItemAbstractGuiMetadata::Flags(), []( QgsLayout *layout ) -> QgsLayoutItem * {
    auto shape = std::make_unique<QgsLayoutItemShape>( layout );
    shape->setShapeType( QgsLayoutItemShape::Rectangle );
    return shape.release();
  } ) );
  registry->addLayoutItemGuiMetadata( new QgsLayoutItemGuiMetadata( QgsLayoutItemRegistry::LayoutShape, QObject::tr( "Ellipse" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddBasicCircle.svg" ) ), createShapeWidget, createEllipseBand, QStringLiteral( "shapes" ), false, QgsLayoutItemAbstractGuiMetadata::Flags(), []( QgsLayout *layout ) -> QgsLayoutItem * {
    auto shape = std::make_unique<QgsLayoutItemShape>( layout );
    shape->setShapeType( QgsLayoutItemShape::Ellipse );
    return shape.release();
  } ) );
  registry->addLayoutItemGuiMetadata( new QgsLayoutItemGuiMetadata( QgsLayoutItemRegistry::LayoutShape, QObject::tr( "Triangle" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddBasicTriangle.svg" ) ), createShapeWidget, createTriangleBand, QStringLiteral( "shapes" ), false, QgsLayoutItemAbstractGuiMetadata::Flags(), []( QgsLayout *layout ) -> QgsLayoutItem * {
    auto shape = std::make_unique<QgsLayoutItemShape>( layout );
    shape->setShapeType( QgsLayoutItemShape::Triangle );
    return shape.release();
  } ) );

  // marker
  registry->addLayoutItemGuiMetadata( new QgsLayoutItemGuiMetadata( QgsLayoutItemRegistry::LayoutMarker, QObject::tr( "Marker" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddMarker.svg" ) ), []( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * { return new QgsLayoutMarkerWidget( qobject_cast<QgsLayoutItemMarker *>( item ) ); }, nullptr ) );

  // arrow
  auto arrowMetadata = std::make_unique<QgsLayoutItemGuiMetadata>(
    QgsLayoutItemRegistry::LayoutPolyline, QObject::tr( "Arrow" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddArrow.svg" ) ),
    []( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * {
      return new QgsLayoutPolylineWidget( qobject_cast<QgsLayoutItemPolyline *>( item ) );
    },
    createRubberBand, QString(), true
  );
  arrowMetadata->setItemCreationFunction( []( QgsLayout *layout ) -> QgsLayoutItem * {
    auto arrow = std::make_unique<QgsLayoutItemPolyline>( layout );
    arrow->setEndMarker( QgsLayoutItemPolyline::ArrowHead );
    return arrow.release();
  } );
  arrowMetadata->setNodeRubberBandCreationFunction( []( QgsLayoutView * ) -> QGraphicsItemGroup * {
    auto band = std::make_unique<QGraphicsItemGroup>();
    QGraphicsPathItem *poly = new QGraphicsPathItem( band.get() );
    poly->setPen( QPen( QBrush( QColor( 227, 22, 22, 200 ) ), 0 ) );

    QGraphicsPathItem *tempPoly = new QGraphicsPathItem( band.get() );
    tempPoly->setPen( QPen( QBrush( QColor( 227, 22, 22, 200 ) ), 0, Qt::DotLine ) );

    band->setZValue( QgsLayout::ZViewTool );
    return band.release();
  } );
  registry->addLayoutItemGuiMetadata( arrowMetadata.release() );

  // node items

  auto polygonMetadata = std::make_unique<QgsLayoutItemGuiMetadata>(
    QgsLayoutItemRegistry::LayoutPolygon, QObject::tr( "Polygon" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddPolygon.svg" ) ),
    []( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * {
      return new QgsLayoutPolygonWidget( qobject_cast<QgsLayoutItemPolygon *>( item ) );
    },
    createRubberBand, QStringLiteral( "nodes" ), true
  );
  polygonMetadata->setNodeRubberBandCreationFunction( []( QgsLayoutView * ) -> QGraphicsItemGroup * {
    auto band = std::make_unique<QGraphicsItemGroup>();
    QGraphicsPolygonItem *poly = new QGraphicsPolygonItem( band.get() );
    poly->setBrush( QBrush( QColor( 227, 22, 22, 20 ) ) );
    poly->setPen( QPen( QBrush( QColor( 227, 22, 22, 200 ) ), 0 ) );

    QGraphicsPolygonItem *tempPoly = new QGraphicsPolygonItem( band.get() );
    tempPoly->setBrush( Qt::NoBrush );
    tempPoly->setPen( QPen( QBrush( QColor( 227, 22, 22, 200 ) ), 0, Qt::DotLine ) );

    band->setZValue( QgsLayout::ZViewTool );
    return band.release();
  } );
  registry->addLayoutItemGuiMetadata( polygonMetadata.release() );

  auto polylineMetadata = std::make_unique<QgsLayoutItemGuiMetadata>(
    QgsLayoutItemRegistry::LayoutPolyline, QObject::tr( "Polyline" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddPolyline.svg" ) ),
    []( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * {
      return new QgsLayoutPolylineWidget( qobject_cast<QgsLayoutItemPolyline *>( item ) );
    },
    createRubberBand, QStringLiteral( "nodes" ), true
  );
  polylineMetadata->setNodeRubberBandCreationFunction( []( QgsLayoutView * ) -> QGraphicsItemGroup * {
    auto band = std::make_unique<QGraphicsItemGroup>();
    QGraphicsPathItem *poly = new QGraphicsPathItem( band.get() );
    poly->setPen( QPen( QBrush( QColor( 227, 22, 22, 200 ) ), 0 ) );

    QGraphicsPathItem *tempPoly = new QGraphicsPathItem( band.get() );
    tempPoly->setPen( QPen( QBrush( QColor( 227, 22, 22, 200 ) ), 0, Qt::DotLine ) );

    band->setZValue( QgsLayout::ZViewTool );
    return band.release();
  } );
  registry->addLayoutItemGuiMetadata( polylineMetadata.release() );


  // html item

  auto htmlItemMetadata = std::make_unique<QgsLayoutItemGuiMetadata>( QgsLayoutItemRegistry::LayoutHtml, QObject::tr( "HTML" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddHtml.svg" ) ), []( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * { return new QgsLayoutHtmlWidget( qobject_cast<QgsLayoutFrame *>( item ) ); }, createRubberBand );
  htmlItemMetadata->setItemCreationFunction( []( QgsLayout *layout ) -> QgsLayoutItem * {
    auto htmlMultiFrame = std::make_unique<QgsLayoutItemHtml>( layout );
    QgsLayoutItemHtml *html = htmlMultiFrame.get();
    layout->addMultiFrame( htmlMultiFrame.release() );
    auto frame = std::make_unique<QgsLayoutFrame>( layout, html );
    QgsLayoutFrame *f = frame.get();
    html->addFrame( frame.release() );
    // cppcheck-suppress returnDanglingLifetime
    return f;
  } );
  registry->addLayoutItemGuiMetadata( htmlItemMetadata.release() );

  // attribute table item

  auto attributeTableItemMetadata = std::make_unique<QgsLayoutItemGuiMetadata>( QgsLayoutItemRegistry::LayoutAttributeTable, QObject::tr( "Attribute Table" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddTable.svg" ) ), []( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * { return new QgsLayoutAttributeTableWidget( qobject_cast<QgsLayoutFrame *>( item ) ); }, createRubberBand );
  attributeTableItemMetadata->setItemCreationFunction( []( QgsLayout *layout ) -> QgsLayoutItem * {
    auto tableMultiFrame = std::make_unique<QgsLayoutItemAttributeTable>( layout );
    QgsLayoutItemAttributeTable *table = tableMultiFrame.get();

    //set first vector layer from layer registry as table source
    QMap<QString, QgsMapLayer *> layerMap = layout->project()->mapLayers();
    for ( auto it = layerMap.constBegin(); it != layerMap.constEnd(); ++it )
    {
      if ( QgsVectorLayer *vl = qobject_cast<QgsVectorLayer *>( it.value() ) )
      {
        table->setVectorLayer( vl );
        break;
      }
    }

    //set default table fonts from settings
    QgsSettings settings;
    const QString defaultFontString = settings.value( QStringLiteral( "LayoutDesigner/defaultFont" ), QVariant(), QgsSettings::Gui ).toString();
    if ( !defaultFontString.isEmpty() )
    {
      QgsTextFormat format;
      QFont f = format.font();
      QgsFontUtils::setFontFamily( f, defaultFontString );
      format.setFont( f );
      tableMultiFrame->setContentTextFormat( format );
      f.setBold( true );
      format.setFont( f );
      tableMultiFrame->setHeaderTextFormat( format );
    }

    layout->addMultiFrame( tableMultiFrame.release() );
    auto frame = std::make_unique<QgsLayoutFrame>( layout, table );
    QgsLayoutFrame *f = frame.get();
    table->addFrame( frame.release() );
    // cppcheck-suppress returnDanglingLifetime
    return f;
  } );
  registry->addLayoutItemGuiMetadata( attributeTableItemMetadata.release() );

  // manual table item

  auto manualTableItemMetadata = std::make_unique<QgsLayoutItemGuiMetadata>( QgsLayoutItemRegistry::LayoutManualTable, QObject::tr( "Fixed Table" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionAddManualTable.svg" ) ), []( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * { return new QgsLayoutManualTableWidget( qobject_cast<QgsLayoutFrame *>( item ) ); }, createRubberBand );
  manualTableItemMetadata->setItemCreationFunction( []( QgsLayout *layout ) -> QgsLayoutItem * {
    auto tableMultiFrame = std::make_unique<QgsLayoutItemManualTable>( layout );
    QgsLayoutItemManualTable *table = tableMultiFrame.get();

    // initially start with a 2x2 empty table
    QgsTableContents contents;
    contents << ( QgsTableRow() << QgsTableCell() << QgsTableCell() );
    contents << ( QgsTableRow() << QgsTableCell() << QgsTableCell() );
    table->setTableContents( contents );

    //set default table fonts from settings
    QgsSettings settings;
    const QString defaultFontString = settings.value( QStringLiteral( "LayoutDesigner/defaultFont" ), QVariant(), QgsSettings::Gui ).toString();
    if ( !defaultFontString.isEmpty() )
    {
      QgsTextFormat format;
      QFont f = format.font();
      QgsFontUtils::setFontFamily( f, defaultFontString );
      format.setFont( f );
      tableMultiFrame->setContentTextFormat( format );
      f.setBold( true );
      format.setFont( f );
      tableMultiFrame->setHeaderTextFormat( format );
    }

    layout->addMultiFrame( tableMultiFrame.release() );

    auto frame = std::make_unique<QgsLayoutFrame>( layout, table );
    QgsLayoutFrame *f = frame.get();
    table->addFrame( frame.release() );
    // cppcheck-suppress returnDanglingLifetime
    return f;
  } );
  manualTableItemMetadata->setItemDoubleClickedFunction( []( QgsLayoutItem *item, Qgis::MouseHandlesAction ) {
    QgsLayoutManualTableWidget::openTableDesigner( qobject_cast<QgsLayoutFrame *>( item ) );
  } );
  registry->addLayoutItemGuiMetadata( manualTableItemMetadata.release() );


  // elevation profile item

  auto elevationProfileItemMetadata = std::make_unique<QgsLayoutItemGuiMetadata>( QgsLayoutItemRegistry::LayoutElevationProfile, QObject::tr( "Elevation Profile" ), QgsApplication::getThemeIcon( QStringLiteral( "/mActionElevationProfile.svg" ) ), []( QgsLayoutItem *item ) -> QgsLayoutItemBaseWidget * { return new QgsLayoutElevationProfileWidget( qobject_cast<QgsLayoutItemElevationProfile *>( item ) ); }, createRubberBand );
  elevationProfileItemMetadata->setItemCreationFunction( []( QgsLayout *layout ) -> QgsLayoutItem * {
    auto profileItem = std::make_unique<QgsLayoutItemElevationProfile>( layout );

    //set default fonts from settings
    QgsSettings settings;
    const QString defaultFontString = settings.value( QStringLiteral( "LayoutDesigner/defaultFont" ), QVariant(), QgsSettings::Gui ).toString();
    if ( !defaultFontString.isEmpty() )
    {
      QgsTextFormat format = profileItem->plot()->xAxis().textFormat();
      QFont f = format.font();
      QgsFontUtils::setFontFamily( f, defaultFontString );
      format.setFont( f );
      profileItem->plot()->xAxis().setTextFormat( format );

      format = profileItem->plot()->yAxis().textFormat();
      f = format.font();
      QgsFontUtils::setFontFamily( f, defaultFontString );
      format.setFont( f );
      profileItem->plot()->yAxis().setTextFormat( format );
    }
    return profileItem.release();
  } );
  registry->addLayoutItemGuiMetadata( elevationProfileItemMetadata.release() );
}
