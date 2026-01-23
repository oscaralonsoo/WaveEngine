#include "UIModule.h"
#include "Application.h"
#include "Input.h"
#include "Window.h"
#include <imgui.h>

UIModule::UIModule() : Module() { name = "UIModule"; }
UIModule::~UIModule() {}

bool UIModule::Start() {
    LOG_CONSOLE("Module UI V3: HUD y Ventanas Arrastrables activas.");
    return true;
}

bool UIModule::Update()
{

    if (Application::GetInstance().input->GetKey(SDL_SCANCODE_F1) == KEY_DOWN)
        isSettingsVisible = !isSettingsVisible;
        
    RenderHUD(); 

    if (isSettingsVisible) 
        DrawEngineSettings();

    return true;
}

void UIModule::RenderHUD()
{
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 screenCenter = ImVec2(vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f);


    ImDrawList* drawList = ImGui::GetForegroundDrawList();


    ImU32 whiteColor = IM_COL32(255, 255, 255, 200);
    
    drawList->AddLine(
        ImVec2(screenCenter.x - crosshairSize, screenCenter.y),
        ImVec2(screenCenter.x + crosshairSize, screenCenter.y),
        whiteColor, crosshairThickness
    );
    drawList->AddLine(
        ImVec2(screenCenter.x, screenCenter.y - crosshairSize),
        ImVec2(screenCenter.x, screenCenter.y + crosshairSize),
        whiteColor, crosshairThickness
    );
}

void UIModule::DrawEngineSettings()
{
    ImGui::SetNextWindowSize(ImVec2(320, 260), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Game Settings (F1)", &isSettingsVisible))
    {

        ImVec2 pos = ImGui::GetWindowPos();
        ImGuiViewport* vp = ImGui::GetMainViewport();
        

        if (pos.x < vp->Pos.x) ImGui::SetWindowPos(ImVec2(vp->Pos.x, pos.y));
        if (pos.y < vp->Pos.y) ImGui::SetWindowPos(ImVec2(pos.x, vp->Pos.y));

        ImGui::Text("Interface & Graphics");
        ImGui::Separator();

        if (ImGui::Checkbox("VSync (V2 Feature)", &vsyncActive)) {
            Application::GetInstance().window->SetVsync(vsyncActive);
        }

        
        ImGui::Separator();
        ImGui::Text("Crosshair Settings");
        ImGui::SliderFloat("Size", &crosshairSize, 5.0f, 30.0f);
        ImGui::SliderFloat("Thickness", &crosshairThickness, 1.0f, 5.0f);

        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(80, 0))) isSettingsVisible = false;
    }
    ImGui::End();
}