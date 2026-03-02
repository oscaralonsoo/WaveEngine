////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include "UWPDisplay.h"

#include <NsCore/ReflectionImplement.h>
#include <NsCore/Category.h>
#include <NsCore/UTF8.h>
#include <NsCore/Math.h>
#include <NsGui/IntegrationAPI.h>
#include <NsGui/InputEnums.h>
#include <NsGui/TextBox.h>
#include <NsGui/IView.h>
#include <NsGui/Cursor.h>


using namespace Noesis;
using namespace NoesisApp;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::Text::Core;
using namespace Windows::UI::ViewManagement;
using namespace Windows::Devices::Input;
using namespace Windows::Graphics::Display;


static uint8_t gKeyTable[256];
static CoreTextEditContext^ gEditContext;
static TextBox* gIMEFocused;
static bool gActive;
static uint64_t gLastClickTime;
static MouseButton gLastClickButton;
static const uint32_t DoubleClickTime = 500;


////////////////////////////////////////////////////////////////////////////////////////////////////
static MouseButton GetMouseButton(PointerPointProperties^ props)
{
    if (props->PointerUpdateKind == PointerUpdateKind::LeftButtonPressed ||
        props->PointerUpdateKind == PointerUpdateKind::LeftButtonReleased)
    {
        return MouseButton_Left;
    }
    if (props->PointerUpdateKind == PointerUpdateKind::MiddleButtonPressed ||
        props->PointerUpdateKind == PointerUpdateKind::MiddleButtonReleased)
    {
        return MouseButton_Middle;
    }
    if (props->PointerUpdateKind == PointerUpdateKind::RightButtonPressed ||
        props->PointerUpdateKind == PointerUpdateKind::RightButtonReleased)
    {
        return MouseButton_Right;
    }
    if (props->PointerUpdateKind == PointerUpdateKind::XButton1Pressed ||
        props->PointerUpdateKind == PointerUpdateKind::XButton1Released)
    {
        return MouseButton_XButton1;
    }
    if (props->PointerUpdateKind == PointerUpdateKind::XButton2Pressed ||
        props->PointerUpdateKind == PointerUpdateKind::XButton2Released)
    {
        return MouseButton_XButton2;
    }

    return MouseButton_Left;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UWPDisplay::UWPDisplay()
{
    CoreWindow^ window = CoreWindow::GetForCurrentThread();
    window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(
        [&](CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
    {
        const Windows::Foundation::Rect& r = sender->Bounds;
        float scale = (float)DisplayInformation::GetForCurrentView()->RawPixelsPerViewPixel;
        mSizeChanged(this, (uint32_t)(scale * r.Width), (uint32_t)(scale * r.Height));
    });

    window->Activated += ref new TypedEventHandler<CoreWindow^, WindowActivatedEventArgs^>(
        [&](CoreWindow^ sender, WindowActivatedEventArgs^ args)
    {
        if (args->WindowActivationState == CoreWindowActivationState::Deactivated)
        {
            mDeactivated(this);
            gActive = false;
        }
        else
        {
            mActivated(this);
            gActive = true;
        }
    });

    window->KeyDown += ref new TypedEventHandler<CoreWindow^, Windows::UI::Core::KeyEventArgs^>(
        [&](CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
    {
        if ((uint8_t)args->VirtualKey <= 0xff && gKeyTable[(uint8_t)args->VirtualKey] != 0)
        {
            mKeyDown(this, (Key)gKeyTable[(uint8_t)args->VirtualKey]);
        }
    });

    window->KeyUp += ref new TypedEventHandler<CoreWindow^, Windows::UI::Core::KeyEventArgs^>(
        [&](CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
    {
        if ((uint8_t)args->VirtualKey <= 0xff && gKeyTable[(uint8_t)args->VirtualKey] != 0)
        {
            mKeyUp(this, (Key)gKeyTable[(uint8_t)args->VirtualKey]);
        }
    });

    window->CharacterReceived += ref new TypedEventHandler<CoreWindow^, CharacterReceivedEventArgs^>(
        [&](CoreWindow^ sender, CharacterReceivedEventArgs^ args)
    {
        if (gIMEFocused == nullptr)
        {
            mChar(this, args->KeyCode);
        }
    });

    window->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
        [&](CoreWindow^ sender, PointerEventArgs^ args)
    {
        PointerPoint^ p = args->CurrentPoint;
        PointerDevice^ device = p->PointerDevice;
        float scale = (float)DisplayInformation::GetForCurrentView()->RawPixelsPerViewPixel;
        int x = (int)(scale * p->Position.X);
        int y = (int)(scale * p->Position.Y);

        if (device->PointerDeviceType == PointerDeviceType::Mouse)
        {
            mMouseMove(this, x, y);
        }
        else
        {
            mTouchMove(this, x, y, p->PointerId);
        }
    });

    window->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
        [&](CoreWindow^ sender, PointerEventArgs^ args)
    {
        PointerPoint^ p = args->CurrentPoint;
        PointerDevice^ device = p->PointerDevice;
        float scale = (float)DisplayInformation::GetForCurrentView()->RawPixelsPerViewPixel;
        int x = (int)(scale * p->Position.X);
        int y = (int)(scale * p->Position.Y);

        if (device->PointerDeviceType == PointerDeviceType::Mouse)
        {
            MouseButton button = GetMouseButton(p->Properties);
            uint64_t time = p->Timestamp;
            uint64_t delta = (time - gLastClickTime) / 1000;

            if (delta <= DoubleClickTime && button == gLastClickButton)
            {
                mMouseDoubleClick(this, x, y, button);
                gLastClickTime = 0;
            }
            else
            {
                mMouseButtonDown(this, x, y, button); 
                gLastClickTime = time;
            }

            gLastClickButton = button;
        }
        else
        {
            mTouchDown(this, x, y, p->PointerId);
        }
    });

    window->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
        [&](CoreWindow^ sender, PointerEventArgs^ args)
    {
        PointerPoint^ p = args->CurrentPoint;
        PointerDevice^ device = p->PointerDevice;
        float scale = (float)DisplayInformation::GetForCurrentView()->RawPixelsPerViewPixel;
        int x = (int)(scale * p->Position.X);
        int y = (int)(scale * p->Position.Y);

        if (device->PointerDeviceType == PointerDeviceType::Mouse)
        {
            mMouseButtonUp(this, x, y, GetMouseButton(p->Properties)); 
        }
        else
        {
            mTouchUp(this, x, y, p->PointerId);
        }
    });

    window->PointerWheelChanged += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(
        [&](CoreWindow^ sender, PointerEventArgs^ args)
    {
        PointerPoint^ p = args->CurrentPoint;
        float scale = (float)DisplayInformation::GetForCurrentView()->RawPixelsPerViewPixel;
        int x = (int)(scale * p->Position.X);
        int y = (int)(scale * p->Position.Y);
        int delta = p->Properties->MouseWheelDelta;

        if (p->Properties->IsHorizontalMouseWheel)
        {
            mMouseHWheel(this, x, y, delta);
        }
        else
        {
            mMouseWheel(this, x, y, delta);
        }
    });

#define MAP_KEY(vk, k) \
    static_assert((uint8_t)VirtualKey::vk <= 255, ""); gKeyTable[(uint8_t)VirtualKey::vk] = k

    MAP_KEY(Back, Key_Back);
    MAP_KEY(Tab, Key_Tab);
    MAP_KEY(Clear, Key_Clear);
    MAP_KEY(Enter, Key_Return);
    MAP_KEY(Pause, Key_Pause);
    MAP_KEY(NumberKeyLock, Key_NumLock);
    MAP_KEY(Scroll, Key_Scroll);

    MAP_KEY(Shift, Key_LeftShift);
    MAP_KEY(LeftShift, Key_LeftShift);
    MAP_KEY(RightShift, Key_RightShift);
    MAP_KEY(Control, Key_LeftCtrl);
    MAP_KEY(LeftControl, Key_LeftCtrl);
    MAP_KEY(RightControl, Key_RightCtrl);
    MAP_KEY(Menu, Key_LeftAlt);
    MAP_KEY(LeftMenu, Key_LeftAlt);
    MAP_KEY(RightMenu, Key_RightAlt);
    MAP_KEY(LeftWindows, Key_LWin);
    MAP_KEY(RightWindows, Key_RWin);
    MAP_KEY(Escape, Key_Escape);

    MAP_KEY(Space, Key_Space);
    MAP_KEY(PageUp, Key_Prior);
    MAP_KEY(PageDown, Key_Next);
    MAP_KEY(End, Key_End);
    MAP_KEY(Home, Key_Home);
    MAP_KEY(Left, Key_Left);
    MAP_KEY(Up, Key_Up);
    MAP_KEY(Right, Key_Right);
    MAP_KEY(Down, Key_Down);
    MAP_KEY(Select, Key_Select);
    MAP_KEY(Print, Key_Print);
    MAP_KEY(Execute, Key_Execute);
    MAP_KEY(Snapshot, Key_Snapshot);
    MAP_KEY(Insert, Key_Insert);
    MAP_KEY(Delete, Key_Delete);
    MAP_KEY(Help, Key_Help);

    MAP_KEY(Number0, Key_D0);
    MAP_KEY(Number1, Key_D1);
    MAP_KEY(Number2, Key_D2);
    MAP_KEY(Number3, Key_D3);
    MAP_KEY(Number4, Key_D4);
    MAP_KEY(Number5, Key_D5);
    MAP_KEY(Number6, Key_D6);
    MAP_KEY(Number7, Key_D7);
    MAP_KEY(Number8, Key_D8);
    MAP_KEY(Number9, Key_D9);

    MAP_KEY(NumberPad0, Key_NumPad0);
    MAP_KEY(NumberPad1, Key_NumPad1);
    MAP_KEY(NumberPad2, Key_NumPad2);
    MAP_KEY(NumberPad3, Key_NumPad3);
    MAP_KEY(NumberPad4, Key_NumPad4);
    MAP_KEY(NumberPad5, Key_NumPad5);
    MAP_KEY(NumberPad6, Key_NumPad6);
    MAP_KEY(NumberPad7, Key_NumPad7);
    MAP_KEY(NumberPad8, Key_NumPad8);
    MAP_KEY(NumberPad9, Key_NumPad9);

    MAP_KEY(GamepadA, Key_GamepadAccept);
    MAP_KEY(GamepadB, Key_GamepadCancel);
    MAP_KEY(GamepadX, Key_GamepadContext1);
    MAP_KEY(GamepadY, Key_GamepadContext2);
    MAP_KEY(GamepadRightShoulder, Key_GamepadPageRight);
    MAP_KEY(GamepadLeftShoulder, Key_GamepadPageLeft);
    MAP_KEY(GamepadLeftTrigger, Key_GamepadPageUp);
    MAP_KEY(GamepadRightTrigger, Key_GamepadPageDown);
    MAP_KEY(GamepadDPadUp, Key_GamepadUp);
    MAP_KEY(GamepadDPadDown, Key_GamepadDown);
    MAP_KEY(GamepadDPadLeft, Key_GamepadLeft);
    MAP_KEY(GamepadDPadRight, Key_GamepadRight);
    MAP_KEY(GamepadMenu, Key_GamepadMenu);
    MAP_KEY(GamepadView, Key_GamepadView);
    MAP_KEY(GamepadLeftThumbstickButton, Key_GamepadContext3);
    MAP_KEY(GamepadRightThumbstickButton, Key_GamepadContext4);
    MAP_KEY(GamepadLeftThumbstickUp, Key_GamepadUp);
    MAP_KEY(GamepadLeftThumbstickDown, Key_GamepadDown);
    MAP_KEY(GamepadLeftThumbstickLeft, Key_GamepadLeft);
    MAP_KEY(GamepadLeftThumbstickRight, Key_GamepadRight);
    MAP_KEY(GamepadRightThumbstickUp, Key_GamepadPageUp);
    MAP_KEY(GamepadRightThumbstickDown, Key_GamepadPageDown);
    MAP_KEY(GamepadRightThumbstickLeft, Key_GamepadPageLeft);
    MAP_KEY(GamepadRightThumbstickRight, Key_GamepadPageRight);

    MAP_KEY(GoHome, Key_BrowserHome);
    MAP_KEY(Search, Key_BrowserSearch);
    MAP_KEY(Favorites, Key_BrowserFavorites);
    MAP_KEY(GoBack, Key_BrowserBack);
    MAP_KEY(GoForward, Key_BrowserForward);
    MAP_KEY(Refresh, Key_BrowserRefresh);

    MAP_KEY(Multiply, Key_Multiply);
    MAP_KEY(Add, Key_Add);
    MAP_KEY(Separator, Key_Separator);
    MAP_KEY(Subtract, Key_Subtract);
    MAP_KEY(Decimal, Key_Decimal);
    MAP_KEY(Divide, Key_Divide);

    MAP_KEY(A, Key_A);
    MAP_KEY(B, Key_B);
    MAP_KEY(C, Key_C);
    MAP_KEY(D, Key_D);
    MAP_KEY(E, Key_E);
    MAP_KEY(F, Key_F);
    MAP_KEY(G, Key_G);
    MAP_KEY(H, Key_H);
    MAP_KEY(I, Key_I);
    MAP_KEY(J, Key_J);
    MAP_KEY(K, Key_K);
    MAP_KEY(L, Key_L);
    MAP_KEY(M, Key_M);
    MAP_KEY(N, Key_N);
    MAP_KEY(O, Key_O);
    MAP_KEY(P, Key_P);
    MAP_KEY(Q, Key_Q);
    MAP_KEY(R, Key_R);
    MAP_KEY(S, Key_S);
    MAP_KEY(T, Key_T);
    MAP_KEY(U, Key_U);
    MAP_KEY(V, Key_V);
    MAP_KEY(W, Key_W);
    MAP_KEY(X, Key_X);
    MAP_KEY(Y, Key_Y);
    MAP_KEY(Z, Key_Z);

    MAP_KEY(F1, Key_F1);
    MAP_KEY(F2, Key_F2);
    MAP_KEY(F3, Key_F3);
    MAP_KEY(F4, Key_F4);
    MAP_KEY(F5, Key_F5);
    MAP_KEY(F6, Key_F6);
    MAP_KEY(F7, Key_F7);
    MAP_KEY(F8, Key_F8);
    MAP_KEY(F9, Key_F9);
    MAP_KEY(F10, Key_F10);
    MAP_KEY(F11, Key_F11);
    MAP_KEY(F12, Key_F12);
    MAP_KEY(F13, Key_F13);
    MAP_KEY(F14, Key_F14);
    MAP_KEY(F15, Key_F15);
    MAP_KEY(F16, Key_F16);
    MAP_KEY(F17, Key_F17);
    MAP_KEY(F18, Key_F18);
    MAP_KEY(F19, Key_F19);
    MAP_KEY(F20, Key_F20);
    MAP_KEY(F21, Key_F21);
    MAP_KEY(F22, Key_F22);
    MAP_KEY(F23, Key_F23);
    MAP_KEY(F24, Key_F24);

    CoreTextServicesManager^ manager = CoreTextServicesManager::GetForCurrentView();
    gEditContext = manager->CreateEditContext();
    gEditContext->InputPaneDisplayPolicy = CoreTextInputPaneDisplayPolicy::Manual;

    // The system raises this event to request a specific range of text
    gEditContext->TextRequested += ref new TypedEventHandler<CoreTextEditContext^,CoreTextTextRequestedEventArgs^>(
        [](CoreTextEditContext^ context, CoreTextTextRequestedEventArgs^ args)
    {
        const char* text = gIMEFocused->GetText();

        uint16_t text_[512];
        UTF8::UTF8To16(text, text_, 512);

        int start = args->Request->Range.StartCaretPosition;
        int end = args->Request->Range.EndCaretPosition;
        int len = Min(end, (int)wcslen((wchar_t*)text_)) - start;

        args->Request->Text = ref new Platform::String((wchar_t*)text_ + start, len);
    });

    // Return the current selection
    gEditContext->SelectionRequested += ref new TypedEventHandler<CoreTextEditContext^, CoreTextSelectionRequestedEventArgs^>(  
        [](CoreTextEditContext^ context, CoreTextSelectionRequestedEventArgs^ args)
    {
        CoreTextRange range;
        range.StartCaretPosition = gIMEFocused->GetSelectionStart();
        range.EndCaretPosition = range.StartCaretPosition + gIMEFocused->GetSelectionLength();
        args->Request->Selection = range;
    });

    // The system raises this event to update text in the edit control
    gEditContext->TextUpdating += ref new TypedEventHandler<CoreTextEditContext^, CoreTextTextUpdatingEventArgs^>(
        [](CoreTextEditContext^ context, CoreTextTextUpdatingEventArgs^ args)
    {
        char newText_[512];
        UTF8::UTF16To8((const uint16_t*)args->Text->Data(), newText_, 512);

        uint32_t start = args->Range.StartCaretPosition;
        uint32_t end = args->Range.EndCaretPosition;
        gIMEFocused->Select(start, end - start);
        gIMEFocused->SetSelectedText(newText_);

        uint32_t start_ = args->NewSelection.StartCaretPosition;
        uint32_t end_ = args->NewSelection.EndCaretPosition;
        gIMEFocused->Select(start_, end_ - start_);
    });

    // The system raises this event to change the selection in the edit control
    gEditContext->SelectionUpdating += ref new TypedEventHandler<CoreTextEditContext^, CoreTextSelectionUpdatingEventArgs^>(
        [](CoreTextEditContext^ context, CoreTextSelectionUpdatingEventArgs^ args)
    {
        uint32_t start = args->Selection.StartCaretPosition;
        uint32_t end = args->Selection.EndCaretPosition;
        gIMEFocused->Select(start, end - start);
    });

    // The system raises this event when it wants the edit control to apply formatting on a range of text
    gEditContext->FormatUpdating += ref new TypedEventHandler<CoreTextEditContext^, CoreTextFormatUpdatingEventArgs^>(
        [](CoreTextEditContext^ context, CoreTextFormatUpdatingEventArgs^ args)
    {
        gIMEFocused->ClearCompositionUnderlines();

        if (args->UnderlineType != nullptr)
        {
            CompositionUnderline u =
            {   
                (uint32_t)args->Range.StartCaretPosition,
                (uint32_t)args->Range.EndCaretPosition,
                CompositionLineStyle_Solid,
                false
            };

            switch (args->UnderlineType->Value)
            {
                case Windows::UI::Text::UnderlineType::Dash:
                case Windows::UI::Text::UnderlineType::DashDot:
                case Windows::UI::Text::UnderlineType::DashDotDot:
                case Windows::UI::Text::UnderlineType::LongDash:
                {
                    u.style = CompositionLineStyle_Dash;
                    break;
                }
                case Windows::UI::Text::UnderlineType::ThickDash:
                case Windows::UI::Text::UnderlineType::ThickDashDot:
                case Windows::UI::Text::UnderlineType::ThickLongDash:
                {
                    u.style = CompositionLineStyle_Dash;
                    u.bold = true;
                    break;
                }
                case Windows::UI::Text::UnderlineType::Dotted:
                {
                    u.style = CompositionLineStyle_Dot;
                    break;
                }
                case Windows::UI::Text::UnderlineType::ThickDotted:
                {
                    u.style = CompositionLineStyle_Dot;
                    u.bold = true;
                    break;
                }
                case Windows::UI::Text::UnderlineType::Wave:
                {
                    u.style = CompositionLineStyle_Squiggle;
                    break;
                }
                case Windows::UI::Text::UnderlineType::DoubleWave:
                case Windows::UI::Text::UnderlineType::HeavyWave:
                {
                    u.style = CompositionLineStyle_Squiggle;
                    u.bold = true;
                    break;
                }
                default:
                    break;
            }

            gIMEFocused->AddCompositionUnderline(u);
        }
    });

    // The system raises this event to request layout information.
    // This is used to help choose a position for the IME candidate window.
    gEditContext->LayoutRequested += ref new TypedEventHandler<CoreTextEditContext^, CoreTextLayoutRequestedEventArgs^>(
        [](CoreTextEditContext^ context, CoreTextLayoutRequestedEventArgs^ args)
    {
        Visual* visual = gIMEFocused->GetTextView();
        Matrix4 m = visual->TransformToAncestor(gIMEFocused->GetView()->GetContent());

        uint32_t start = gIMEFocused->GetSelectionStart();
        uint32_t end = start + gIMEFocused->GetSelectionLength();
        Noesis::Rect selectionRect = gIMEFocused->GetRangeBounds(start, end);
        selectionRect.height += Max(1.0f, gIMEFocused->GetFontSize() * 0.1f);
        selectionRect.Transform(m);

        Noesis::Rect contentRect(Noesis::Size(gIMEFocused->GetActualWidth(), gIMEFocused->GetActualHeight()));
        contentRect.Transform(m);

        Windows::Foundation::Rect windowRect = CoreWindow::GetForCurrentThread()->Bounds;
        float scale = (float)DisplayInformation::GetForCurrentView()->RawPixelsPerViewPixel;

        // This is the bounds of the selection.
        args->Request->LayoutBounds->TextBounds = Windows::Foundation::Rect
        (
            scale * (selectionRect.x + windowRect.X),
            scale * (selectionRect.y + windowRect.Y),
            scale * selectionRect.width,
            scale * selectionRect.height
        );

        // This is the bounds of the whole control
        args->Request->LayoutBounds->ControlBounds = Windows::Foundation::Rect
        (
            scale * (contentRect.x + windowRect.X),
            scale * (contentRect.y + windowRect.Y),
            scale * contentRect.width,
            scale * contentRect.height
        );
    });

    // The system raises this event to notify the edit control that the string composition has started
    gEditContext->CompositionStarted += ref new TypedEventHandler<CoreTextEditContext^, CoreTextCompositionStartedEventArgs^>(
        [](CoreTextEditContext^ context, CoreTextCompositionStartedEventArgs^ args)
    {
        gIMEFocused->ClearCompositionUnderlines();
    });

    // The system raises this event to notify the edit control that the string composition is finished
    gEditContext->CompositionCompleted += ref new TypedEventHandler<CoreTextEditContext^, CoreTextCompositionCompletedEventArgs^>(
        [](CoreTextEditContext^ context, CoreTextCompositionCompletedEventArgs^ args)
    {
        gIMEFocused->ClearCompositionUnderlines();
    });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UWPDisplay::~UWPDisplay()
{
    gEditContext = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UWPDisplay::Show()
{
    mLocationChanged(this, 0, 0);
    mSizeChanged(this, GetClientWidth(), GetClientHeight());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UWPDisplay::EnterMessageLoop(bool runInBackground)
{
    for (;;)
    {
        bool inspector = GUI::IsInspectorConnected();
        bool sleep = !inspector && !runInBackground && !gActive;
        CoreDispatcher^ dispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;

        if (sleep)
        {
            dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
        }
        else
        {
            dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
        }

        mRender(this);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void OnSelectionChanged(BaseComponent*, const RoutedEventArgs&)
{
    CoreTextRange r;
    r.StartCaretPosition = gIMEFocused->GetSelectionStart();
    r.EndCaretPosition = r.StartCaretPosition + gIMEFocused->GetSelectionLength();
    gEditContext->NotifySelectionChanged(r);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UWPDisplay::OpenSoftwareKeyboard(Noesis::UIElement* focused)
{
    InputPane::GetForCurrentView()->TryShow();
    gIMEFocused = DynamicCast<TextBox*>(focused);

    if (gIMEFocused != nullptr) 
    {
        switch (gIMEFocused->GetInputScope())
        {
            case InputScope_Url:
            {
                gEditContext->InputScope = CoreTextInputScope::Url;
                break;
            }
            case InputScope_FullFilePath:
            {
                gEditContext->InputScope = CoreTextInputScope::FilePath;
                break;
            }
            case InputScope_FileName:
            {
                gEditContext->InputScope = CoreTextInputScope::FileName;
                break;
            }
            case InputScope_EmailUserName:
            {
                gEditContext->InputScope = CoreTextInputScope::EmailUserName;
                break;
            }
            case InputScope_EmailSmtpAddress:
            {
                gEditContext->InputScope = CoreTextInputScope::EmailAddress;
                break;
            }
            case InputScope_LogOnName:
            {
                gEditContext->InputScope = CoreTextInputScope::UserName;
                break;
            }
            case InputScope_PersonalFullName:
            {
                gEditContext->InputScope = CoreTextInputScope::PersonalFullName;
                break;
            }
            case InputScope_PersonalNamePrefix:
            {
                gEditContext->InputScope = CoreTextInputScope::PersonalNamePrefix;
                break;
            }
            case InputScope_PersonalGivenName:
            {
                gEditContext->InputScope = CoreTextInputScope::PersonalGivenName;
                break;
            }
            case InputScope_PersonalMiddleName:
            {
                gEditContext->InputScope = CoreTextInputScope::PersonalMiddleName;
                break;
            }
            case InputScope_PersonalSurname:
            {
                gEditContext->InputScope = CoreTextInputScope::PersonalSurname;
                break;
            }
            case InputScope_PersonalNameSuffix:
            {
            {
                gEditContext->InputScope = CoreTextInputScope::PersonalNameSuffix;
                break;
            }
            }
            case InputScope_PostalAddress:
            {
                gEditContext->InputScope = CoreTextInputScope::Address;
                break;
            }
            case InputScope_PostalCode:
            {
                gEditContext->InputScope = CoreTextInputScope::AddressPostalCode;
                break;
            }
            case InputScope_AddressStreet:
            {
                gEditContext->InputScope = CoreTextInputScope::AddressStreet;
                break;
            }
            case InputScope_AddressStateOrProvince:
            {
                gEditContext->InputScope = CoreTextInputScope::AddressStateOrProvince;
                break;
            }
            case InputScope_AddressCity:
            {
                gEditContext->InputScope = CoreTextInputScope::AddressCity;
                break;
            }
            case InputScope_AddressCountryName:
            {
                gEditContext->InputScope = CoreTextInputScope::AddressCountryName;
                break;
            }
            case InputScope_AddressCountryShortName:
            {
                gEditContext->InputScope = CoreTextInputScope::AddressCountryShortName;
                break;
            }
            case InputScope_CurrencyAmountAndSymbol:
            {
                gEditContext->InputScope = CoreTextInputScope::CurrencyAmountAndSymbol;
                break;
            }
            case InputScope_CurrencyAmount:
            {
                gEditContext->InputScope = CoreTextInputScope::CurrencyAmount;
                break;
            }
            case InputScope_Date:
            {
                gEditContext->InputScope = CoreTextInputScope::Date;
                break;
            }
            case InputScope_DateMonth:
            {
                gEditContext->InputScope = CoreTextInputScope::DateMonth;
                break;
            }
            case InputScope_DateDay:
            {
                gEditContext->InputScope = CoreTextInputScope::DateDay;
                break;
            }
            case InputScope_DateYear:
            {
                gEditContext->InputScope = CoreTextInputScope::DateYear;
                break;
            }
            case InputScope_DateMonthName:
            {
                gEditContext->InputScope = CoreTextInputScope::DateMonthName;
                break;
            }
            case InputScope_DateDayName:
            {
                gEditContext->InputScope = CoreTextInputScope::DateDayName;
                break;
            }
            case InputScope_Digits:
            case InputScope_Number:
            {
                gEditContext->InputScope = CoreTextInputScope::Number;
                break;
            }
            case InputScope_OneChar:
            {
                gEditContext->InputScope = CoreTextInputScope::SingleCharacter;
                break;
            }
            case InputScope_Password:
            {
                gEditContext->InputScope = CoreTextInputScope::Password;
                break;
            }
            case InputScope_TelephoneNumber:
            {
                gEditContext->InputScope = CoreTextInputScope::TelephoneNumber;
                break;
            }
            case InputScope_TelephoneCountryCode:
            {
                gEditContext->InputScope = CoreTextInputScope::TelephoneCountryCode;
                break;
            }
            case InputScope_TelephoneAreaCode:
            {
                gEditContext->InputScope = CoreTextInputScope::TelephoneAreaCode;
                break;
            }
            case InputScope_TelephoneLocalNumber:
            {
                gEditContext->InputScope = CoreTextInputScope::TelephoneLocalNumber;
                break;
            }
            case InputScope_Time:
            {
                gEditContext->InputScope = CoreTextInputScope::Time;
                break;
            }
            case InputScope_TimeHour:
            {
                gEditContext->InputScope = CoreTextInputScope::TimeHour;
                break;
            }
            case InputScope_TimeMinorSec:
            {
                gEditContext->InputScope = CoreTextInputScope::TimeMinuteOrSecond;
                break;
            }
            case InputScope_NumberFullWidth:
            {
                gEditContext->InputScope = CoreTextInputScope::NumberFullWidth;
                break;
            }
            case InputScope_AlphanumericHalfWidth:
            {
                gEditContext->InputScope = CoreTextInputScope::AlphanumericHalfWidth;
                break;
            }
            case InputScope_AlphanumericFullWidth:
            {
                gEditContext->InputScope = CoreTextInputScope::AlphanumericFullWidth;
                break;
            }
            case InputScope_CurrencyChinese:
            {
                gEditContext->InputScope = CoreTextInputScope::CurrencyChinese;
                break;
            }
            case InputScope_Bopomofo:
            {
                gEditContext->InputScope = CoreTextInputScope::Bopomofo;
                break;
            }
            case InputScope_Hiragana:
            {
                gEditContext->InputScope = CoreTextInputScope::Hiragana;
                break;
            }
            case InputScope_KatakanaHalfWidth:
            {
                gEditContext->InputScope = CoreTextInputScope::KatakanaHalfWidth;
                break;
            }
            case InputScope_KatakanaFullWidth:
            {
                gEditContext->InputScope = CoreTextInputScope::KatakanaFullWidth;
                break;
            }
            case InputScope_Hanja:
            {
                gEditContext->InputScope = CoreTextInputScope::Hanja;
                break;
            }
            default:
            {
                gEditContext->InputScope = CoreTextInputScope::Default;
                break;
            }
        }

        // Opening the IME is enqueued for the next frame. If this is not done the TAB used
        // for getting focus is recevied by CoreTextEditContext as the first char input. This is
        // happening because this function is invoked in the middle of a KeyDown that will generate
        // a Char event after. Probably a better solution is invoking the OpenSoftwareKeyboard
        // callback after processing input events
        gIMEFocused->GetView()->CreateTimer(0, []() -> uint32_t
        {
            if (gIMEFocused)
            {
                CoreTextRange r;
                r.StartCaretPosition = gIMEFocused->GetSelectionStart();
                r.EndCaretPosition = r.StartCaretPosition + gIMEFocused->GetSelectionLength();
                gEditContext->NotifySelectionChanged(r);

                gIMEFocused->SelectionChanged() += OnSelectionChanged;
                gEditContext->NotifyFocusEnter();
            }
            return 0;
        });
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UWPDisplay::CloseSoftwareKeyboard()
{
    InputPane::GetForCurrentView()->TryHide();

    if (gIMEFocused != nullptr)
    {
        gEditContext->NotifyFocusLeave();

        gIMEFocused->SelectionChanged() -= OnSelectionChanged;
        gIMEFocused = nullptr;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UWPDisplay::SetCursor(Noesis::Cursor* cursor)
{
    CoreWindow^ window = CoreWindow::GetForCurrentThread();

    switch (cursor->Type())
    {
        case Noesis::CursorType_None:
        {
            window->PointerCursor = nullptr;
            break;
        }
        case Noesis::CursorType_No:
        {
            window->PointerCursor = ref new CoreCursor(CoreCursorType::UniversalNo, 0);
            break;
        }
        case Noesis::CursorType_Cross:
        {
            window->PointerCursor = ref new CoreCursor(CoreCursorType::Cross, 0);
            break;
        }
        case Noesis::CursorType_Help:
        {
            window->PointerCursor = ref new CoreCursor(CoreCursorType::Help, 0);
            break;
        }
        case Noesis::CursorType_IBeam:
        {
            window->PointerCursor = ref new CoreCursor(CoreCursorType::IBeam, 0);
            break;
        }
        case Noesis::CursorType_SizeAll:
        {
            window->PointerCursor = ref new CoreCursor(CoreCursorType::SizeAll, 0);
            break;
        }
        case Noesis::CursorType_SizeNESW:
        {
            window->PointerCursor = ref new CoreCursor(CoreCursorType::SizeNortheastSouthwest, 0);
            break;
        }
        case Noesis::CursorType_SizeNS:
        {
            window->PointerCursor = ref new CoreCursor(CoreCursorType::SizeNorthSouth, 0);
            break;
        }
        case Noesis::CursorType_SizeNWSE:
        {
            window->PointerCursor = ref new CoreCursor(CoreCursorType::SizeNorthwestSoutheast, 0);
            break;
        }
        case Noesis::CursorType_SizeWE:
        {
            window->PointerCursor = ref new CoreCursor(CoreCursorType::SizeWestEast, 0);
            break;
        }
        case Noesis::CursorType_UpArrow:
        {
            window->PointerCursor = ref new CoreCursor(CoreCursorType::UpArrow, 0);
            break;
        }
        case Noesis::CursorType_Wait:
        {
            window->PointerCursor = ref new CoreCursor(CoreCursorType::Wait, 0);
            break;
        }
        case Noesis::CursorType_Hand:
        {
            window->PointerCursor = ref new CoreCursor(CoreCursorType::Hand, 0);
            break;
        }
        default:
        {
            window->PointerCursor = ref new CoreCursor(CoreCursorType::Arrow, 0);
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UWPDisplay::OpenUrl(const char* url)
{
    uint16_t url_[512];
    UTF8::UTF8To16(url, url_, 512);

    Launcher::LaunchUriAsync(ref new Windows::Foundation::Uri(ref new Platform::String((wchar_t*)url_)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* UWPDisplay::GetNativeHandle() const
{
    return (void*)(CoreWindow::GetForCurrentThread());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t UWPDisplay::GetClientWidth() const
{
    float scale = (float)DisplayInformation::GetForCurrentView()->RawPixelsPerViewPixel;
    return (uint32_t)(scale * CoreWindow::GetForCurrentThread()->Bounds.Width);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t UWPDisplay::GetClientHeight() const
{
    float scale = (float)DisplayInformation::GetForCurrentView()->RawPixelsPerViewPixel;
    return (uint32_t)(scale * CoreWindow::GetForCurrentThread()->Bounds.Height);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UWPDisplay::GetScale() const
{
    return (float)DisplayInformation::GetForCurrentView()->RawPixelsPerViewPixel;
}

NS_IMPLEMENT_REFLECTION(UWPDisplay)
{
    NsMeta<Category>("Display");
}
