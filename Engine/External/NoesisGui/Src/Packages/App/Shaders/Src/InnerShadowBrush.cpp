  ////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include "InnerShadowBrush.h"

#include <NsCore/ReflectionImplement.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UIPropertyMetadata.h>
#include <NsGui/GradientStopCollection.h>
#include <NsGui/ContentPropertyMetaData.h>

#ifdef HAVE_CUSTOM_SHADERS
#include "InnerShadow.bin.h"
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
InnerShadowBrush::InnerShadowBrush()
{
  #ifdef HAVE_CUSTOM_SHADERS
    RenderContext::EnsureShaders(Shaders, "InnerShadow", InnerShadow_bin);
  #endif

    SetConstantBuffer(&mConstants, sizeof(mConstants));

    for (uint32_t i = 0; i < NS_COUNTOF(Shaders.shaders); i++)
    {
        SetPixelShader(Shaders.shaders[i], (BrushShader::Target)i);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void InnerShadowBrush::SetColor(const Color& color)
{
    SetValue<Color>(ColorProperty, color);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void InnerShadowBrush::SetWidth(float width)
{
    SetValue<float>(WidthProperty, width);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void InnerShadowBrush::SetHeight(float height)
{
    SetValue<float>(HeightProperty, height);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void InnerShadowBrush::SetCornerRadius(float r)
{
    SetValue<float>(CornerRadiusProperty, r);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void InnerShadowBrush::SetBlurRadius(float r)
{
    SetValue<float>(BlurRadiusProperty, r);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void InnerShadowBrush::SetSpread(float spread)
{
    SetValue<float>(SpreadProperty, spread);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void InnerShadowBrush::SetOffsetX(float offset)
{
    SetValue<float>(OffsetXProperty, offset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void InnerShadowBrush::SetOffsetY(float offset)
{
    SetValue<float>(OffsetYProperty, offset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(InnerShadowBrush, "NoesisGUIExtensions.InnerShadowBrush")
{
    auto OnColorChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        InnerShadowBrush* this_ = (InnerShadowBrush*)o;
        this_->mConstants.color = args.NewValue<Color>();
        this_->InvalidateConstantBuffer();
    };

    auto OnWidthChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        InnerShadowBrush* this_ = (InnerShadowBrush*)o;
        this_->mConstants.width = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    auto OnHeightChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        InnerShadowBrush* this_ = (InnerShadowBrush*)o;
        this_->mConstants.height = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    auto OnCornerRadiusChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        InnerShadowBrush* this_ = (InnerShadowBrush*)o;
        this_->mConstants.cornerRadius = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    auto OnBlurRadiusChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        InnerShadowBrush* this_ = (InnerShadowBrush*)o;
        this_->mConstants.blurRadius = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    auto OnSpreadChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        InnerShadowBrush* this_ = (InnerShadowBrush*)o;
        this_->mConstants.spread = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    auto OnOffsetXChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        InnerShadowBrush* this_ = (InnerShadowBrush*)o;
        this_->mConstants.offsetX = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    auto OnOffsetYChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        InnerShadowBrush* this_ = (InnerShadowBrush*)o;
        this_->mConstants.offsetY = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<Color>(ColorProperty, "Color",
        UIPropertyMetadata::Create(Color::White(), PropertyChangedCallback(OnColorChanged)));
    data->RegisterProperty<float>(WidthProperty, "Width",
        UIPropertyMetadata::Create(1.0f, PropertyChangedCallback(OnWidthChanged)));
    data->RegisterProperty<float>(HeightProperty, "Height",
        UIPropertyMetadata::Create(1.0f, PropertyChangedCallback(OnHeightChanged)));
    data->RegisterProperty<float>(CornerRadiusProperty, "CornerRadius",
        UIPropertyMetadata::Create(0.0f, PropertyChangedCallback(OnCornerRadiusChanged)));
    data->RegisterProperty<float>(BlurRadiusProperty, "BlurRadius",
        UIPropertyMetadata::Create(0.0f, PropertyChangedCallback(OnBlurRadiusChanged)));
    data->RegisterProperty<float>(SpreadProperty, "Spread",
        UIPropertyMetadata::Create(0.0f, PropertyChangedCallback(OnSpreadChanged)));
    data->RegisterProperty<float>(OffsetXProperty, "OffsetX",
        UIPropertyMetadata::Create(0.0f, PropertyChangedCallback(OnOffsetXChanged)));
    data->RegisterProperty<float>(OffsetYProperty, "OffsetY",
        UIPropertyMetadata::Create(0.0f, PropertyChangedCallback(OnOffsetYChanged)));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* InnerShadowBrush::ColorProperty;
const DependencyProperty* InnerShadowBrush::WidthProperty;
const DependencyProperty* InnerShadowBrush::HeightProperty;
const DependencyProperty* InnerShadowBrush::CornerRadiusProperty;
const DependencyProperty* InnerShadowBrush::BlurRadiusProperty;
const DependencyProperty* InnerShadowBrush::SpreadProperty;
const DependencyProperty* InnerShadowBrush::OffsetXProperty;
const DependencyProperty* InnerShadowBrush::OffsetYProperty;

BrushShaders InnerShadowBrush::Shaders;
