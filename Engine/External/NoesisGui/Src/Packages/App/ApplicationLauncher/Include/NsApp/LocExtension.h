////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_LOCEXTENSION_H__
#define __APP_LOCEXTENSION_H__


#include <NsCore/Noesis.h>
#include <NsCore/String.h>
#include <NsCore/Ptr.h>
#include <NsGui/MarkupExtension.h>
#include <NsGui/IValueConverter.h>
#include <NsApp/ApplicationLauncherApi.h>


namespace Noesis
{
struct Uri;
class DependencyObject;
class ResourceDictionary;
class DependencyProperty;
}

namespace NoesisApp
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Implements a markup extension that supports references to a localization ResourceDictionary.
///
/// Provides a value for any XAML property attribute by looking up a reference in the
/// ResourceDictionary defined by the Source attached property. Values will be re-evaluated when
/// the Source attached property changes.
///
/// If used with a string or object property, and the provided resource key is not found, the
/// LocExtension will return a string in the format "<Loc !%s>" where %s is replaced with the key.
///
/// A Converter can also be specified, with an optional ConverterParameter.
///
/// This example shows the full setup for a LocExtension. It utilizes the RichText attached
/// property to support BBCode markup in the localized strings. The Loc.Source property references
/// the "Language_en-gb.xaml" ResourceDictionary below.
///
/// .. code-block:: xml
///
///    <StackPanel
///      xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
///      xmlns:noesis="clr-namespace:NoesisGUIExtensions"
///      noesis:Loc.Source="Language_en-gb.xaml">
///      <Image Source="{noesis:Loc Flag}"/>
///      <TextBlock noesis:RichText.Text="{noesis:Loc SoundLabel}"/>
///      <TextBlock noesis:RichText.Text="{noesis:Loc TitleLabel, Converter={StaticResource CaseConverter}, ConverterParameter=UpperCase}"/>
///    </StackPanel>
///
/// This is the contents of a "Language_en-gb.xaml" localized ResourceDictionary:
///
/// .. code-block:: xml
///    :caption: Language_en-gb.xaml
///
///    <ResourceDictionary
///      xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
///      xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
///      xmlns:sys="clr-namespace:System;assembly=mscorlib">
///      <ImageBrush x:Key="Flag" ImageSource="Flag_en-gb.png" Stretch="Fill"/>
///      <sys:String x:Key="TitleLabel">[b]Localization Sample[/b]</sys:String>
///      <sys:String x:Key="SoundLabel">A [i]sound[/i] label</sys:String>
///    </ResourceDictionary>
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_APPLICATIONLAUNCHER_API LocExtension: public Noesis::MarkupExtension
{
public:
    LocExtension() = default;
    LocExtension(const char* key);

    /// Gets the dictionary where localization resources are found
    static Noesis::ResourceDictionary* GetResources(const Noesis::DependencyObject* element);
    void SetResources(Noesis::DependencyObject* element, Noesis::ResourceDictionary* resources);

    /// Gets or sets the value of the Source property of the localization dictionary
    //@{
    static const Noesis::Uri& GetSource(const Noesis::DependencyObject* element);
    static void SetSource(Noesis::DependencyObject* element, const Noesis::Uri& source);
    //@}

    /// Gets or sets the value of the key to use when finding a resource
    //@{
    const char* GetResourceKey() const;
    void SetResourceKey(const char* key);
    //@}

    /// Gets or sets a converter to use on any found resource
    //@{
    Noesis::IValueConverter* GetConverter() const;
    void SetConverter(Noesis::IValueConverter* value);
    //@}

    /// Gets or sets the value of the parameter to use with the converter
    //@{
    BaseComponent* GetConverterParameter() const;
    void SetConverterParameter(BaseComponent* value);
    //@}

    /// From MarkupExtension
    //@{
    Noesis::Ptr<BaseComponent> ProvideValue(const Noesis::ValueTargetProvider* provider) override;
    //@}

public:
    static const Noesis::DependencyProperty* ResourcesProperty;
    static const Noesis::DependencyProperty* SourceProperty;

private:
    Noesis::FixedString<128> mResourceKey;
    Noesis::Ptr<Noesis::IValueConverter> mConverter;
    Noesis::Ptr<BaseComponent> mConverterParameter;

    NS_DECLARE_REFLECTION(LocExtension, MarkupExtension)
};

NS_WARNING_POP

}


#endif
