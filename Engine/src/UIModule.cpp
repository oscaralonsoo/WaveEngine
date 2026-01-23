#include "UIModule.h"
#include "Application.h"
#include "Input.h"
#include "Window.h"
#include <imgui.h>

ModuleUI::ModuleUI() : Module()
{
    name = "ModuleUI";
}

ModuleUI::~ModuleUI() {}

bool ModuleUI::Start()
{
    LOG_CONSOLE("Module UI V2: Sistema de opciones inicializado.");
    return true;
}

bool ModuleUI::Update()
{

    if (Application::GetInstance().input->GetKey(SDL_SCANCODE_F1) == KEY_DOWN)
    {
        isSettingsVisible = !isSettingsVisible;
    }


    if (isSettingsVisible) 
    {
        DrawEngineSettings();
    }

    return true;
}

void ModuleUI::DrawEngineSettings()
{

    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);


    if (ImGui::Begin("Engine Settings (F1)", &isSettingsVisible))
    {
        ImGui::Text("Ajustes de Video");
        ImGui::Separator();


        if (ImGui::Checkbox("Activar VSync", &vsyncActive))
        {
         
            Application::GetInstance().window->SetVsync(vsyncActive);
        }


        ImGui::Separator();
        ImGui::Text("Usuario: %s", userTag);

        if (ImGui::Button("Cerrar"))
        {
            isSettingsVisible = false;
        }
    }
    ImGui::End();
}