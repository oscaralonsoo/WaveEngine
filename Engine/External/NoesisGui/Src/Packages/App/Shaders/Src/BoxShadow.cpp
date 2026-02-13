  ////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/BoxShadow.h>
#include <NsGui/ImageBrush.h>
#include <NsGui/UIElementData.h>
#include <NsGui/DrawingContext.h>
#include <NsGui/FrameworkPropertyMetadata.h>

#include "OuterShadowBrush.h"
#include "InnerShadowBrush.h"

#ifdef NS_HAVE_STUDIO
#include <NsGui/StudioMeta.h>
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
BoxShadow::BoxShadow()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
BoxShadow::~BoxShadow()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const Color& BoxShadow::GetColor() const
{
    return GetValue<Color>(ColorProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void BoxShadow::SetColor(const Color& color)
{
    SetValue<Color>(ColorProperty, color);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float BoxShadow::GetCornerRadius() const
{
    return GetValue<float>(CornerRadiusProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void BoxShadow::SetCornerRadius(float r)
{
    SetValue<float>(CornerRadiusProperty, r);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float BoxShadow::GetBlurRadius() const
{
    return GetValue<float>(BlurRadiusProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void BoxShadow::SetBlurRadius(float r)
{
    SetValue<float>(BlurRadiusProperty, r);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float BoxShadow::GetSpread() const
{
    return GetValue<float>(SpreadProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void BoxShadow::SetSpread(float v)
{
    SetValue<float>(SpreadProperty, v);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float BoxShadow::GetDirection() const
{
    return GetValue<float>(DirectionProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void BoxShadow::SetDirection(float v)
{
    SetValue<float>(DirectionProperty, v);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float BoxShadow::GetDepth() const
{
    return GetValue<float>(DepthProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void BoxShadow::SetDepth(float v)
{
     SetValue<float>(DepthProperty, v);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool BoxShadow::GetInside() const
{
    return GetValue<bool>(InsideProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void BoxShadow::SetInside(bool v)
{
    SetValue<bool>(InsideProperty, v);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void BoxShadow::OnRender(DrawingContext* context)
{
    Color color = GetColor();
    Size size = GetRenderSize();
    float corner = Min(Max(0.0f, GetCornerRadius()), Min(size.width, size.height));
    float radius = Max(0.0f, GetBlurRadius());
    float depth = Max(0.0f, GetDepth());
    float direction = GetDirection();
    float spread = GetSpread();
    bool inside = GetInside();

    float offsetX = cosf(-direction * DegToRad) * depth;
    float offsetY = sinf(-direction * DegToRad) * depth;

    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = size.width;
    float y1 = size.height;

    if (NS_UNLIKELY(!mBrush))
    {
        mBrush = *new ImageBrush();
    }

    if (inside)
    {
        if (NS_UNLIKELY(!mInnerShader))
        {
            mInnerShader = *new InnerShadowBrush();
        }

        mInnerShader->SetColor(color);
        mInnerShader->SetWidth(size.width);
        mInnerShader->SetHeight(size.height);
        mInnerShader->SetBlurRadius(radius);
        mInnerShader->SetCornerRadius(corner);
        mInnerShader->SetSpread(spread);
        mInnerShader->SetOffsetX(-offsetX);
        mInnerShader->SetOffsetY(-offsetY);

        mBrush->SetShader(mInnerShader);
    }
    else
    {
        x0 += offsetX - radius - spread;
        y0 += offsetY - radius - spread;
        x1 += offsetX + radius + spread;
        y1 += offsetY + radius + spread;

        if (NS_UNLIKELY(!mOuterShader))
        {
            mOuterShader = *new OuterShadowBrush();
        }

        mOuterShader->SetColor(color);
        mOuterShader->SetWidth(x1 - x0);
        mOuterShader->SetHeight(y1 - y0);
        mOuterShader->SetBlurRadius(radius);
        mOuterShader->SetCornerRadius(corner + spread);

        mBrush->SetShader(mOuterShader);
    }

    context->DrawRectangle(mBrush, nullptr, Rect(x0, y0, x1, y1));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(BoxShadow, "NoesisGUIExtensions.BoxShadow")
{
    TypeOf<InnerShadowBrush>();
    TypeOf<OuterShadowBrush>();

  #ifdef NS_HAVE_STUDIO
    NsMeta<StudioOrder>(2001, "Shape");
    NsMeta<StudioDesc>("Renders a blurred rounded rectangle");
    //NsMeta<StudioHelpUri>("https://www.noesisengine.com/docs/App.Toolkit._RegularPolygon.html");
    //NsMeta<StudioIcon>(Uri::Pack("Toolkit", "#ToolkitIcons"), 0xE907);

    NsProp("Inside", &BoxShadow::GetInside, &BoxShadow::SetInside)
        .Meta<StudioOrder>(0);
    NsProp("CornerRadius", &BoxShadow::GetCornerRadius, &BoxShadow::SetCornerRadius)
        .Meta<StudioOrder>(1)
        .Meta<StudioMin>(0.0f);
    NsProp("BlurRadius", &BoxShadow::GetBlurRadius, &BoxShadow::SetBlurRadius)
        .Meta<StudioOrder>(2)
        .Meta<StudioMin>(0.0f);
    NsProp("Spread", &BoxShadow::GetSpread, &BoxShadow::SetSpread)
        .Meta<StudioOrder>(3);
    NsProp("Direction", &BoxShadow::GetDirection, &BoxShadow::SetDirection)
        .Meta<StudioOrder>(4);
    NsProp("Depth", &BoxShadow::GetDepth, &BoxShadow::SetDepth)
        .Meta<StudioOrder>(5)
        .Meta<StudioMin>(0.0f);
    NsProp("Color", &BoxShadow::GetColor, &BoxShadow::SetColor)
        .Meta<StudioOrder>(6);
  #endif

    UIElementData* data = NsMeta<UIElementData>(TypeOf<SelfClass>());
    data->RegisterProperty<Color>(ColorProperty, "Color",
        FrameworkPropertyMetadata::Create(Color::Black(), FrameworkPropertyMetadataOptions_AffectsRender));
    data->RegisterProperty<float>(SpreadProperty, "Spread",
        FrameworkPropertyMetadata::Create(0.0f, FrameworkPropertyMetadataOptions_AffectsRender));
    data->RegisterProperty<float>(CornerRadiusProperty, "CornerRadius",
        FrameworkPropertyMetadata::Create(0.0f, FrameworkPropertyMetadataOptions_AffectsRender));
    data->RegisterProperty<float>(BlurRadiusProperty, "BlurRadius",
        FrameworkPropertyMetadata::Create(0.0f, FrameworkPropertyMetadataOptions_AffectsRender));
    data->RegisterProperty<float>(DirectionProperty, "Direction",
        FrameworkPropertyMetadata::Create(315.0f, FrameworkPropertyMetadataOptions_AffectsRender));
    data->RegisterProperty<float>(DepthProperty, "Depth",
        FrameworkPropertyMetadata::Create(0.0f, FrameworkPropertyMetadataOptions_AffectsRender));
    data->RegisterProperty<bool>(InsideProperty, "Inside",
        FrameworkPropertyMetadata::Create(false, FrameworkPropertyMetadataOptions_AffectsRender));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* BoxShadow::ColorProperty;
const DependencyProperty* BoxShadow::CornerRadiusProperty;
const DependencyProperty* BoxShadow::BlurRadiusProperty;
const DependencyProperty* BoxShadow::SpreadProperty;
const DependencyProperty* BoxShadow::DirectionProperty;
const DependencyProperty* BoxShadow::DepthProperty;
const DependencyProperty* BoxShadow::InsideProperty;
