////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_BOXSHADOW_H__
#define __APP_BOXSHADOW_H__


#include <NsApp/ShadersApi.h>
#include <NsGui/FrameworkElement.h>


namespace Noesis
{
struct Color;
class ImageBrush;
}

namespace NoesisApp
{

class InnerShadowBrush;
class OuterShadowBrush;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// A box for rendering blurred rounded rectangles.
///
/// This primitive uses a BrushShader internally to efficiently render blurred, rounded rectangles,
/// supporting both inner and outer shadow effects. Unlike traditional effect-based approaches,
/// it avoids the overhead of offscreen rendering, making it a faster alternative.
///
/// .. code-block:: xml
///
///  <Grid
///    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
///    xmlns:noesis="clr-namespace:NoesisGUIExtensions;assembly=Noesis.GUI.Extensions"
///    Background="White">
///    <StackPanel Orientation="Horizontal">
///      <noesis:BoxShadow Width="300" Height="200" BlurRadius="50" />
///      <noesis:BoxShadow Width="300" Height="200" BlurRadius="100" />
///      <noesis:BoxShadow Width="300" Height="200" BlurRadius="200" />
///    </StackPanel>
///  </Grid>
///
/// .. image:: RectShadow.jpg
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_SHADERS_API BoxShadow final: public Noesis::FrameworkElement
{
public:
    BoxShadow();
    ~BoxShadow();

    /// Gets or sets the color of the rectangle
    //@{
    const Noesis::Color& GetColor() const;
    void SetColor(const Noesis::Color& color);
    //@}

    /// Gets or sets the radius that is used to round the corners of the rectangle
    //@{
    float GetCornerRadius() const;
    void SetCornerRadius(float r);
    //@}

    /// Gets or sets a value that indicates the radius of the shadow's blur effect
    //@{
    float GetBlurRadius() const;
    void SetBlurRadius(float r);
    //@}

    /// Positive values will cause the shadow to expand and grow bigger, negative values
    /// will cause the shadow to shrink
    //@{
    float GetSpread() const;
    void SetSpread(float v);
    //@}

    /// Gets or sets the direction in degrees of the drop shadow.
    //@{
    float GetDirection() const;
    void SetDirection(float v);
    //@}

    /// Gets or sets the distance of the shadow below
    //@{
    float GetDepth() const;
    void SetDepth(float v);
    //@}

    /// Changes the shadow from an outer box-shadow to an inner box-shadow
    //@{
    bool GetInside() const;
    void SetInside(bool v);
    //@}

private:
    /// From UIElement
    //@{
    void OnRender(Noesis::DrawingContext* context) override;
    //@}

private:
    Noesis::Ptr<Noesis::ImageBrush> mBrush;
    Noesis::Ptr<InnerShadowBrush> mInnerShader;
    Noesis::Ptr<OuterShadowBrush> mOuterShader;

    static const Noesis::DependencyProperty* ColorProperty;
    static const Noesis::DependencyProperty* CornerRadiusProperty;
    static const Noesis::DependencyProperty* BlurRadiusProperty;
    static const Noesis::DependencyProperty* SpreadProperty;
    static const Noesis::DependencyProperty* DirectionProperty;
    static const Noesis::DependencyProperty* DepthProperty;
    static const Noesis::DependencyProperty* InsideProperty;

    NS_DECLARE_REFLECTION(BoxShadow, FrameworkElement)
};

NS_WARNING_POP

}

#endif
