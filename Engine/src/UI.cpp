#include "UI.h"
#include "NoesisPCH.h"
#include "NsCore/Noesis.h"
#include "NsApp/LocalFontProvider.h"
#include "NsApp/LocalXamlProvider.h"
#include "NsApp/LocalTextureProvider.h"
#include "NsGui/IView.h"
#include "NsGui/FrameworkElement.h"
#include "NsGui/IntegrationAPI.h"  
#include "GLRenderDevice.h" 

UI::UI() { LOG_DEBUG("UI Constructor"); }
UI::~UI() {}

bool UI::Start()
{
    // --- Logger ---
    Noesis::SetLogHandler([](const char*, uint32_t, uint32_t level, const char*, const char* msg)
    {
        const char* prefixes[] = { "T", "D", "I", "W", "E" };
        LOG_DEBUG("[NOESIS/%s] %s\n", prefixes[level], msg);
    });

    // --- Init ---
    Noesis::GUI::SetLicense(NS_LICENSE_NAME, NS_LICENSE_KEY);
    Noesis::GUI::Init();

    Noesis::GUI::SetXamlProvider(Noesis::MakePtr<NoesisApp::LocalXamlProvider>("."));
    Noesis::GUI::SetFontProvider(Noesis::MakePtr<NoesisApp::LocalFontProvider>("."));
    Noesis::GUI::SetTextureProvider(Noesis::MakePtr<NoesisApp::LocalTextureProvider>("."));

    // --- Cargar el XAML ---
    Noesis::Ptr<Noesis::FrameworkElement> xaml =
        Noesis::GUI::LoadXaml<Noesis::FrameworkElement>("Assets/UI/MainHUD.xaml");

    if (!xaml)
    {
        LOG_DEBUG("[UI] Failed to load XAML");
        return false;
    }

    // --- Crear la View ---
    Noesis::Ptr<Noesis::RenderDevice> device = Noesis::MakePtr<NoesisApp::GLRenderDevice>(false);
    m_view = Noesis::GUI::CreateView(xaml);
    m_view->SetFlags(Noesis::RenderFlags_PPAA | Noesis::RenderFlags_LCD);

    m_view->SetSize(1280, 720);

    m_view->GetRenderer()->Init(device);

    return true;
}

bool UI::PreUpdate()
{
    return true;
}

bool UI::Update()
{
    

    return true;
}

bool UI::PostUpdate()
{
    if (!m_view) return true;
    m_view->Update(0.0);


    // Render offscreen
    m_view->GetRenderer()->UpdateRenderTree();
    m_view->GetRenderer()->RenderOffscreen();

    m_view->GetRenderer()->Render();
    return true;
}

bool UI::CleanUp()
{
    if (m_view)
    {
        m_view->GetRenderer()->Shutdown();
        m_view.Reset();
    }

    Noesis::GUI::Shutdown();
    return true;
}

void UI::OnResize(uint32_t width, uint32_t height)
{
    if (m_view)
        m_view->SetSize(width, height);
}