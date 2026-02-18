#include "UI.h"

#include "NoesisPCH.h"
#include "NsCore/Noesis.h"
#include "NsApp/LocalFontProvider.h"
#include "NsApp/LocalXamlProvider.h"
#include "NsApp/LocalTextureProvider.h"


UI::UI()
{
	LOG_DEBUG("UI Constructor");
}

UI::~UI()
{

}


bool UI::Start()
{
    Noesis::SetLogHandler([](const char*, uint32_t, uint32_t level, const char*, const char* msg)
    {
            // [TRACE] [DEBUG] [INFO] [WARNING] [ERROR]
            const char* prefixes[] = { "T", "D", "I", "W", "E" };
            LOG_DEBUG("[NOESIS/%s] %s\n", prefixes[level], msg);
    });

    Noesis::GUI::SetLicense(NS_LICENSE_NAME, NS_LICENSE_KEY);

    Noesis::GUI::Init();

    Noesis::GUI::SetXamlProvider(Noesis::MakePtr<NoesisApp::LocalXamlProvider>("."));
    Noesis::GUI::SetFontProvider(Noesis::MakePtr<NoesisApp::LocalFontProvider>("."));
    Noesis::GUI::SetTextureProvider(Noesis::MakePtr<NoesisApp::LocalTextureProvider>("."));

    return true;
}

bool UI::Update()
{
    return true;
}

bool UI::CleanUp()
{
    return true;
}

bool UI::PreUpdate()
{
    return true;
}