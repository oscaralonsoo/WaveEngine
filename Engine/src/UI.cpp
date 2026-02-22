#include "UI.h"
#include "ComponentCanvas.h"
#include <glad/glad.h>
#include "GLRenderDevice.h"
#include "Application.h"
#include "Time.h"
#include "ModuleEditor.h"
#include "imgui.h"
#include "NoesisPCH.h"
#include "NsCore/Noesis.h"
#include <NsCore/RegisterComponent.h>
#include <NsCore/Package.h>
#include "NsApp/LocalFontProvider.h"
#include "NsApp/LocalXamlProvider.h"
#include "NsApp/LocalTextureProvider.h"
#include <NsApp/EventTrigger.h>
#include <NsApp/GoToStateAction.h>
#include <NsApp/InvokeCommandAction.h>
#include <NsApp/Interaction.h>
#include "NsGui/IView.h"
#include "NsGui/FrameworkElement.h"
#include "NsGui/IntegrationAPI.h"
#include "NsGui/InputEnums.h" 

extern "C" void NsRegisterReflectionAppInteractivity();
extern "C" void NsInitPackageAppInteractivity();
extern "C" void NsShutdownPackageAppInteractivity();

UI::UI() { LOG_DEBUG("UI Constructor"); }
UI::~UI() {}

bool UI::Start()
{
    Noesis::SetLogHandler([](const char*, uint32_t, uint32_t level, const char*, const char* msg)
        {
            const char* prefixes[] = { "T", "D", "I", "W", "E" };
            LOG_DEBUG("[NOESIS/%s] %s\n", prefixes[level], msg);
        });

    Noesis::GUI::SetLicense(NS_LICENSE_NAME, NS_LICENSE_KEY);
    Noesis::GUI::Init();
    NsRegisterReflectionAppInteractivity();
    NsInitPackageAppInteractivity();

    Noesis::GUI::SetXamlProvider(Noesis::MakePtr<NoesisApp::LocalXamlProvider>("../Assets/UI"));
    Noesis::GUI::SetFontProvider(Noesis::MakePtr<NoesisApp::LocalFontProvider>("../Assets/Fonts"));
    Noesis::GUI::SetTextureProvider(Noesis::MakePtr<NoesisApp::LocalTextureProvider>("../Assets/UI/Textures"));

    renderDevice = Noesis::MakePtr<NoesisApp::GLRenderDevice>(false);

    return true;
}

bool UI::CleanUp()
{
    for (ComponentCanvas* c : canvas)
    {
        c->CleanUp();
    }
    canvas.clear();

    NsShutdownPackageAppInteractivity();
    Noesis::GUI::Shutdown();
    renderDevice.Reset();

    return true;
}

void UI::SetMousePosition(int x, int y)
{
    mouseX = x;
    mouseY = y;
    for (ComponentCanvas* c : canvas)
        c->OnMouseMove(x, y);
}

static Noesis::MouseButton SDLButtonToNoesis(int sdlButton)
{
    switch (sdlButton)
    {
    case SDL_BUTTON_LEFT:   return Noesis::MouseButton_Left;
    case SDL_BUTTON_RIGHT:  return Noesis::MouseButton_Right;
    case SDL_BUTTON_MIDDLE: return Noesis::MouseButton_Middle;
    default:                return Noesis::MouseButton_Left;
    }
}

void UI::OnMouseButtonDown(int x, int y, int sdlButton)
{
    for (ComponentCanvas* c : canvas)
        c->OnMouseButtonDown(x, y, SDLButtonToNoesis(sdlButton));
}

void UI::OnMouseButtonUp(int x, int y, int sdlButton)
{
    for (ComponentCanvas* c : canvas)
        c->OnMouseButtonUp(x, y, SDLButtonToNoesis(sdlButton));
}
void UI::OnMouseWheel(int x, int y, int delta) {
    for (ComponentCanvas* c : canvas)
        c->OnMouseWheel(x, y, delta);
}
void UI::OnResize(uint32_t width, uint32_t height) {
    for (ComponentCanvas* c : canvas)
        c->Resize((int)width, (int)height);
}
