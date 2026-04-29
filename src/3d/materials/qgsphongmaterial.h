/***************************************************************************
  qgsphongmaterial.h
  --------------------------------------
  Date                 : April 2026
  Copyright            : (C) 2026 by Dominik Cindrić
  Email                : viper dot miniq at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSPHONGMATERIAL_H
#define QGSPHONGMATERIAL_H

#include "qgis_3d.h"
#include "qgsmaterial.h"

#include <QColor>
#include <QObject>

#define SIP_NO_FILE

namespace Qt3DRender
{
  class QParameter;
  class QShaderProgram;
} // namespace Qt3DRender

///@cond PRIVATE

/**
 * \ingroup qgis_3d
 * \brief A Phong shading material for use in QGIS 3D views.
 * Supports both uniform colors and data-defined per-vertex colors.
 * \since QGIS 4.2
 */
class _3D_EXPORT QgsPhongMaterial : public QgsMaterial
{
    Q_OBJECT

  public:
    /**
     * Constructor for QgsPhongMaterial, with the specified \a parent node.
     */
    explicit QgsPhongMaterial( Qt3DCore::QNode *parent = nullptr );
    ~QgsPhongMaterial() override;

    QColor ambient() const;
    QColor diffuse() const;
    QColor specular() const;
    float shininess() const;
    float opacity() const;

  public slots:
    void setAmbient( const QColor &ambient );
    void setDiffuse( const QColor &diffuse );
    void setSpecular( const QColor &specular );
    void setShininess( float shininess );
    void setOpacity( float opacity );

    /**
     * Switches between data-defined (per-vertex attribute) and uniform color mode.
     * When \a enabled is TRUE, the phongDataDefined.vert shader is used and
     * the DATA_DEFINED define is injected into the fragment shader.
     */
    void setDataDefinedEnabled( bool enabled );

  signals:
    void ambientChanged( const QColor &ambient );
    void diffuseChanged( const QColor &diffuse );
    void specularChanged( const QColor &specular );
    void shininessChanged( float shininess );
    void opacityChanged( float opacity );

  private:
    void init();
    void updateShaders();

    void handleAmbientChanged( const QVariant &var );
    void handleDiffuseChanged( const QVariant &var );
    void handleSpecularChanged( const QVariant &var );
    void handleShininessChanged( const QVariant &var );
    void handleOpacityChanged( const QVariant &var );

    Qt3DRender::QParameter *mAmbientParameter = nullptr;
    Qt3DRender::QParameter *mDiffuseParameter = nullptr;
    Qt3DRender::QParameter *mSpecularParameter = nullptr;
    Qt3DRender::QParameter *mShininessParameter = nullptr;
    Qt3DRender::QParameter *mOpacityParameter = nullptr;
    Qt3DRender::QShaderProgram *mShaderProgram = nullptr;
    bool mDataDefinedEnabled = false;
};

///@endcond PRIVATE

#endif // QGSPHONGMATERIAL_H
