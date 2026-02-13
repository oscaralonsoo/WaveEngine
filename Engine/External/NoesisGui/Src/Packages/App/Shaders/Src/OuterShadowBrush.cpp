  ////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include "OuterShadowBrush.h"

#include <NsCore/ReflectionImplement.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UIPropertyMetadata.h>
#include <NsGui/GradientStopCollection.h>
#include <NsGui/ContentPropertyMetaData.h>

#ifdef HAVE_CUSTOM_SHADERS
#include "OuterShadow.bin.h"
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
OuterShadowBrush::OuterShadowBrush()
{
  #ifdef HAVE_CUSTOM_SHADERS
    RenderContext::EnsureShaders(Shaders, "OuterShadow", OuterShadow_bin);
  #endif

    SetConstantBuffer(&mConstants, sizeof(mConstants));

    for (uint32_t i = 0; i < NS_COUNTOF(Shaders.shaders); i++)
    {
        SetPixelShader(Shaders.shaders[i], (BrushShader::Target)i);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void OuterShadowBrush::SetColor(const Color& color)
{
    SetValue<Color>(ColorProperty, color);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void OuterShadowBrush::SetWidth(float width)
{
    SetValue<float>(WidthProperty, width);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void OuterShadowBrush::SetHeight(float height)
{
    SetValue<float>(HeightProperty, height);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void OuterShadowBrush::SetCornerRadius(float r)
{
    SetValue<float>(CornerRadiusProperty, r);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void OuterShadowBrush::SetBlurRadius(float r)
{
    SetValue<float>(BlurRadiusProperty, r);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(OuterShadowBrush, "NoesisGUIExtensions.OuterShadowBrush")
{
    auto OnColorChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        OuterShadowBrush* this_ = (OuterShadowBrush*)o;
        this_->mConstants.color = args.NewValue<Color>();
        this_->InvalidateConstantBuffer();
    };

    auto OnWidthChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        OuterShadowBrush* this_ = (OuterShadowBrush*)o;
        this_->mConstants.width = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    auto OnHeightChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        OuterShadowBrush* this_ = (OuterShadowBrush*)o;
        this_->mConstants.height = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    auto OnCornerRadiusChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        OuterShadowBrush* this_ = (OuterShadowBrush*)o;
        this_->mConstants.cornerRadius = args.NewValue<float>();
        this_->InvalidateConstantBuffer();
    };

    auto OnBlurRadiusChanged = [](DependencyObject* o, const DependencyPropertyChangedEventArgs& args)
    {
        OuterShadowBrush* this_ = (OuterShadowBrush*)o;
        this_->mConstants.blurRadius = args.NewValue<float>();
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
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* OuterShadowBrush::ColorProperty;
const DependencyProperty* OuterShadowBrush::WidthProperty;
const DependencyProperty* OuterShadowBrush::HeightProperty;
const DependencyProperty* OuterShadowBrush::CornerRadiusProperty;
const DependencyProperty* OuterShadowBrush::BlurRadiusProperty;

BrushShaders OuterShadowBrush::Shaders;
