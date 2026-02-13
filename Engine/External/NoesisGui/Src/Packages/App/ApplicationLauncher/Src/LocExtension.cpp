////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/LocExtension.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/TypeConverter.h>
#include <NsGui/ValueTargetProvider.h>
#include <NsGui/ContentPropertyMetaData.h>
#include <NsGui/DependencyObject.h>
#include <NsGui/FrameworkPropertyMetadata.h>
#include <NsGui/ResourceDictionary.h>
#include <NsGui/UIElementData.h>
#include <NsGui/Uri.h>
#include <NsGui/Binding.h>
#include <NsGui/Enums.h>
#include <NsGui/RelativeSource.h>
#include <NsGui/FrameworkElement.h>


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
LocExtension::LocExtension(const char* key): mResourceKey(key)
{
    if (mResourceKey.Empty())
    {
        NS_ERROR("LocExtension requires a valid string ResourceKey");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ResourceDictionary* LocExtension::GetResources(const DependencyObject* element)
{
    NS_CHECK(element != nullptr, "Element is null");
    return element->GetValue<Ptr<ResourceDictionary>>(ResourcesProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LocExtension::SetResources(DependencyObject* element, ResourceDictionary* resources)
{
    NS_CHECK(element != nullptr, "Element is null");
    element->SetValue<Ptr<ResourceDictionary>>(ResourcesProperty, resources);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const Uri& LocExtension::GetSource(const DependencyObject* element)
{
    NS_CHECK(element != nullptr, "Element is null");
    return element->GetValue<Uri>(SourceProperty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LocExtension::SetSource(DependencyObject* element, const Uri& source)
{
    NS_CHECK(element != nullptr, "Element is null");
    element->SetValue<Uri>(SourceProperty, source);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* LocExtension::GetResourceKey() const
{
    return mResourceKey.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LocExtension::SetResourceKey(const char* key)
{
    mResourceKey = key;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
IValueConverter* LocExtension::GetConverter() const
{
    return mConverter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LocExtension::SetConverter(IValueConverter* value)
{
    mConverter.Reset(value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
BaseComponent* LocExtension::GetConverterParameter() const
{
    return mConverterParameter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LocExtension::SetConverterParameter(BaseComponent* value)
{
    mConverterParameter.Reset(value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<BaseComponent> LocExtension::ProvideValue(const ValueTargetProvider* provider)
{
    Ptr<RelativeSource> source;
    if (DynamicCast<FrameworkElement*>(provider->GetTargetObject()) != nullptr)
    {
        source = *new RelativeSource(RelativeSourceMode_Self);
    }
    else
    {
        source = *new RelativeSource(RelativeSourceMode_FindAncestor,
            TypeOf<FrameworkElement>(), 1);
    }

    Ptr<Binding> binding = *new Binding(String(String::VarArgs(),
        "(NoesisGUIExtensions.Loc.Resources)[%s]", mResourceKey.Str()).Str(), source);
    binding->SetConverter(mConverter);
    binding->SetConverterParameter(mConverterParameter);

    return binding->ProvideValue(provider);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void OnSourceChanged(DependencyObject* obj, const DependencyPropertyChangedEventArgs& args)
{
    const Uri sourceUri = args.NewValue<Uri>();

    if (StrIsNullOrEmpty(sourceUri.Str()))
    {
        obj->ClearLocalValue(LocExtension::ResourcesProperty);
        return;
    }

    const Ptr<ResourceDictionary> resourceDictionary = MakePtr<ResourceDictionary>();
    resourceDictionary->SetSource(sourceUri);

    obj->SetValue<Ptr<ResourceDictionary>>(LocExtension::ResourcesProperty, resourceDictionary);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(LocExtension, "NoesisGUIExtensions.Loc")
{
    NsMeta<ContentPropertyMetaData>("ResourceKey");
    NsProp("ResourceKey", &LocExtension::GetResourceKey, 
        &LocExtension::SetResourceKey);
    NsProp("Converter", &LocExtension::GetConverter,
        &LocExtension::SetConverter);
    NsProp("ConverterParameter", &LocExtension::GetConverterParameter,
        &LocExtension::SetConverterParameter);
    
    DependencyData* data = NsMeta<DependencyData>(TypeOf<SelfClass>());
    data->RegisterProperty<Uri>(SourceProperty, "Source", 
        PropertyMetadata::Create(Uri(), OnSourceChanged));
    data->RegisterProperty<Ptr<ResourceDictionary>>(ResourcesProperty, "Resources",
        FrameworkPropertyMetadata::Create(Ptr<ResourceDictionary>(),
            FrameworkPropertyMetadataOptions_Inherits));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const DependencyProperty* LocExtension::ResourcesProperty;
const DependencyProperty* LocExtension::SourceProperty;
