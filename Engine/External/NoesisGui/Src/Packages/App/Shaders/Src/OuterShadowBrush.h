////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_OUTERSHADOWBRUSH_H__
#define __APP_OUTERSHADOWBRUSH_H__


#include <NsApp/ShadersApi.h>
#include <NsGui/BrushShader.h>
#include <NsDrawing/Color.h>
#include <NsRender/RenderContext.h>


namespace Noesis { class DependencyProperty; }

namespace NoesisApp
{

////////////////////////////////////////////////////////////////////////////////////////////////////
class OuterShadowBrush final: public Noesis::BrushShader
{
public:
    OuterShadowBrush();

    void SetColor(const Noesis::Color& color);
    void SetWidth(float width);
    void SetHeight(float height);
    void SetCornerRadius(float r);
    void SetBlurRadius(float r);

public:
    static const Noesis::DependencyProperty* ColorProperty;
    static const Noesis::DependencyProperty* WidthProperty;
    static const Noesis::DependencyProperty* HeightProperty;
    static const Noesis::DependencyProperty* CornerRadiusProperty;
    static const Noesis::DependencyProperty* BlurRadiusProperty;

    static BrushShaders Shaders;

private:
    struct Constants
    {
        Noesis::Color color = Noesis::Color::White();
        float width = 1.0f;
        float height = 1.0f;
        float cornerRadius = 0.0f;
        float blurRadius = 0.0f;
    };  

    Constants mConstants;

    NS_DECLARE_REFLECTION(OuterShadowBrush, BrushShader)
};

}

#endif
