////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_INNERSHADOWBRUSH_H__
#define __APP_INNERSHADOWBRUSH_H__


#include <NsApp/ShadersApi.h>
#include <NsGui/BrushShader.h>
#include <NsDrawing/Color.h>
#include <NsRender/RenderContext.h>


namespace Noesis { class DependencyProperty; }

namespace NoesisApp
{


////////////////////////////////////////////////////////////////////////////////////////////////////
class InnerShadowBrush final: public Noesis::BrushShader
{
public:
    InnerShadowBrush();

    void SetColor(const Noesis::Color& color);
    void SetWidth(float width);
    void SetHeight(float height);
    void SetCornerRadius(float r);
    void SetBlurRadius(float r);
    void SetSpread(float spread);
    void SetOffsetX(float offset);
    void SetOffsetY(float offset);

public:
    static const Noesis::DependencyProperty* ColorProperty;
    static const Noesis::DependencyProperty* WidthProperty;
    static const Noesis::DependencyProperty* HeightProperty;
    static const Noesis::DependencyProperty* CornerRadiusProperty;
    static const Noesis::DependencyProperty* BlurRadiusProperty;
    static const Noesis::DependencyProperty* SpreadProperty;
    static const Noesis::DependencyProperty* OffsetXProperty;
    static const Noesis::DependencyProperty* OffsetYProperty;

    static BrushShaders Shaders;

private:
    struct Constants
    {
        Noesis::Color color = Noesis::Color::White();
        float width = 1.0f;
        float height = 1.0f;
        float cornerRadius = 0.0f;
        float blurRadius = 0.0f;
        float spread = 0.0f;
        float offsetX = 0.0f;
        float offsetY = 0.0f;
    };  

    Constants mConstants;

    NS_DECLARE_REFLECTION(InnerShadowBrush, BrushShader)
};

}

#endif
