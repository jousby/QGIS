/***************************************************************************
  qgsgoochmaterial.cpp
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

#include "qgsgoochmaterial.h"

#include "qgs3dutils.h"

#include <QString>
#include <QUrl>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QFilterKey>
#include <Qt3DRender/QGraphicsApiFilter>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QShaderProgram>
#include <Qt3DRender/QTechnique>

#include "moc_qgsgoochmaterial.cpp"

using namespace Qt::StringLiterals;

///@cond PRIVATE
QgsGoochMaterial::QgsGoochMaterial( QNode *parent )
  : QgsMaterial( parent )
  , mDiffuseParameter( new Qt3DRender::QParameter( u"kd"_s, QColor::fromRgbF( 0.7f, 0.7f, 0.7f, 1.0f ) ) )
  , mSpecularParameter( new Qt3DRender::QParameter( u"ks"_s, QColor::fromRgbF( 0.01f, 0.01f, 0.01f, 1.0f ) ) )
  , mWarmParameter( new Qt3DRender::QParameter( u"kyellow"_s, QColor::fromRgbF( 1.0f, 1.0f, 0.0f, 1.0f ) ) )
  , mCoolParameter( new Qt3DRender::QParameter( u"kblue"_s, QColor::fromRgbF( 0.0f, 0.0f, 1.0f, 1.0f ) ) )
  , mShininessParameter( new Qt3DRender::QParameter( u"shininess"_s, 150.0f ) )
  , mAlphaParameter( new Qt3DRender::QParameter( u"alpha"_s, 0.25f ) )
  , mBetaParameter( new Qt3DRender::QParameter( u"beta"_s, 0.5f ) )
{
  init();
}

QgsGoochMaterial::~QgsGoochMaterial() = default;

void QgsGoochMaterial::init()
{
  connect( mDiffuseParameter, &Qt3DRender::QParameter::valueChanged, this, &QgsGoochMaterial::handleDiffuseChanged );
  connect( mSpecularParameter, &Qt3DRender::QParameter::valueChanged, this, &QgsGoochMaterial::handleSpecularChanged );
  connect( mWarmParameter, &Qt3DRender::QParameter::valueChanged, this, &QgsGoochMaterial::handleWarmChanged );
  connect( mCoolParameter, &Qt3DRender::QParameter::valueChanged, this, &QgsGoochMaterial::handleCoolChanged );
  connect( mShininessParameter, &Qt3DRender::QParameter::valueChanged, this, &QgsGoochMaterial::handleShininessChanged );
  connect( mAlphaParameter, &Qt3DRender::QParameter::valueChanged, this, &QgsGoochMaterial::handleAlphaChanged );
  connect( mBetaParameter, &Qt3DRender::QParameter::valueChanged, this, &QgsGoochMaterial::handleBetaChanged );

  Qt3DRender::QEffect *effect = new Qt3DRender::QEffect();
  effect->addParameter( mDiffuseParameter );
  effect->addParameter( mSpecularParameter );
  effect->addParameter( mWarmParameter );
  effect->addParameter( mCoolParameter );
  effect->addParameter( mShininessParameter );
  effect->addParameter( mAlphaParameter );
  effect->addParameter( mBetaParameter );

  mShaderProgram = new Qt3DRender::QShaderProgram();

  Qt3DRender::QRenderPass *renderPass = new Qt3DRender::QRenderPass();
  renderPass->setShaderProgram( mShaderProgram );

  Qt3DRender::QFilterKey *filterKey = new Qt3DRender::QFilterKey();
  filterKey->setName( u"renderingStyle"_s );
  filterKey->setValue( u"forward"_s );

  Qt3DRender::QTechnique *technique = new Qt3DRender::QTechnique();
  technique->graphicsApiFilter()->setApi( Qt3DRender::QGraphicsApiFilter::OpenGL );
  technique->graphicsApiFilter()->setProfile( Qt3DRender::QGraphicsApiFilter::CoreProfile );
  technique->graphicsApiFilter()->setMajorVersion( 3 );
  technique->graphicsApiFilter()->setMinorVersion( 3 );
  technique->addFilterKey( filterKey );
  technique->addRenderPass( renderPass );

  effect->addTechnique( technique );
  setEffect( effect );

  updateShaders();
}

void QgsGoochMaterial::updateShaders()
{
  const QByteArray fragCode = Qt3DRender::QShaderProgram::loadSource( QUrl( u"qrc:/shaders/gooch.frag"_s ) );

  if ( mDataDefinedEnabled )
  {
    mShaderProgram->setShaderCode( Qt3DRender::QShaderProgram::Vertex, Qt3DRender::QShaderProgram::loadSource( QUrl( u"qrc:/shaders/goochDataDefined.vert"_s ) ) );
    mShaderProgram->setFragmentShaderCode( Qgs3DUtils::addDefinesToShaderCode( fragCode, QStringList( { u"DATA_DEFINED"_s } ) ) );
  }
  else
  {
    mShaderProgram->setShaderCode( Qt3DRender::QShaderProgram::Vertex, Qt3DRender::QShaderProgram::loadSource( QUrl( u"qrc:/shaders/default.vert"_s ) ) );
    mShaderProgram->setFragmentShaderCode( fragCode );
  }
}

void QgsGoochMaterial::setDataDefinedEnabled( bool enabled )
{
  if ( enabled != mDataDefinedEnabled )
  {
    mDataDefinedEnabled = enabled;
    updateShaders();
  }
}

void QgsGoochMaterial::setDiffuse( const QColor &diffuse )
{
  mDiffuseParameter->setValue( diffuse );
}

void QgsGoochMaterial::setSpecular( const QColor &specular )
{
  mSpecularParameter->setValue( specular );
}

void QgsGoochMaterial::setWarm( const QColor &warm )
{
  mWarmParameter->setValue( warm );
}

void QgsGoochMaterial::setCool( const QColor &cool )
{
  mCoolParameter->setValue( cool );
}

void QgsGoochMaterial::setShininess( float shininess )
{
  mShininessParameter->setValue( shininess );
}

void QgsGoochMaterial::setAlpha( float alpha )
{
  mAlphaParameter->setValue( alpha );
}

void QgsGoochMaterial::setBeta( float beta )
{
  mBetaParameter->setValue( beta );
}

QColor QgsGoochMaterial::diffuse() const
{
  return mDiffuseParameter->value().value<QColor>();
}

QColor QgsGoochMaterial::specular() const
{
  return mSpecularParameter->value().value<QColor>();
}

QColor QgsGoochMaterial::warm() const
{
  return mWarmParameter->value().value<QColor>();
}

QColor QgsGoochMaterial::cool() const
{
  return mCoolParameter->value().value<QColor>();
}

float QgsGoochMaterial::shininess() const
{
  return mShininessParameter->value().toFloat();
}

float QgsGoochMaterial::alpha() const
{
  return mAlphaParameter->value().toFloat();
}

float QgsGoochMaterial::beta() const
{
  return mBetaParameter->value().toFloat();
}

void QgsGoochMaterial::handleDiffuseChanged( const QVariant &var )
{
  emit diffuseChanged( var.value<QColor>() );
}

void QgsGoochMaterial::handleSpecularChanged( const QVariant &var )
{
  emit specularChanged( var.value<QColor>() );
}

void QgsGoochMaterial::handleWarmChanged( const QVariant &var )
{
  emit warmChanged( var.value<QColor>() );
}

void QgsGoochMaterial::handleCoolChanged( const QVariant &var )
{
  emit coolChanged( var.value<QColor>() );
}

void QgsGoochMaterial::handleShininessChanged( const QVariant &var )
{
  emit shininessChanged( var.toFloat() );
}

void QgsGoochMaterial::handleAlphaChanged( const QVariant &var )
{
  emit alphaChanged( var.toFloat() );
}

void QgsGoochMaterial::handleBetaChanged( const QVariant &var )
{
  emit betaChanged( var.toFloat() );
}

///@endcond PRIVATE
