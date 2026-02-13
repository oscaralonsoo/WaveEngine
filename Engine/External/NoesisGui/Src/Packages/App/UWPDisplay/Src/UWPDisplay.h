////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_UWPDISPLAY_H__
#define __GUI_UWPDISPLAY_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsApp/Display.h>


namespace Noesis { class UIElement; }

namespace NoesisApp
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// A window implementation for UWP
////////////////////////////////////////////////////////////////////////////////////////////////////
class UWPDisplay: public Display
{
public:
    UWPDisplay();
    ~UWPDisplay();

    /// From Display
    //@{
    void Show() override;
    void EnterMessageLoop(bool runInBackground) override;
    void OpenSoftwareKeyboard(Noesis::UIElement* focused) override;
    void CloseSoftwareKeyboard() override;
    void SetCursor(Noesis::Cursor* cursor) override;
    void OpenUrl(const char* url) override;
    void* GetNativeHandle() const override;
    uint32_t GetClientWidth() const override;
    uint32_t GetClientHeight() const override;
    float GetScale() const override;
    //@}

private:
    NS_DECLARE_REFLECTION(UWPDisplay, Display)
};

}

#endif
