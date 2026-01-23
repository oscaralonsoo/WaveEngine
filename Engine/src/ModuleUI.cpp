#include "ModuleUI.h"
#include "Application.h"
#include "Input.h"
#include "Time.h"
#include "ModuleResources.h"
#include "ResourceTexture.h"
#include "MetaFile.h"
#include "ModuleCamera.h"
#include "ModuleEditor.h"
#include "ConfigurationWindow.h"
#include "imgui.h"
#include <glad/glad.h>
#include <nlohmann/json.hpp>
#include <fstream>

ModuleUI::ModuleUI() { name = "ModuleUI"; }

void ModuleUI::ResetUI() {
    currentState = UIState::MAIN_MENU;
    fadeAlpha = 0.0f;
}

bool ModuleUI::Start() {
    LoadConfig();
    UID bgUID = MetaFileManager::GetUIDFromAsset("Assets/Textures/menu_bg.png");
    if (bgUID != 0) {
        ResourceTexture* bgRes = static_cast<ResourceTexture*>(Application::GetInstance().resources->RequestResource(bgUID));
        if (bgRes) backgroundTextureID = bgRes->GetGPU_ID();
    }
    return true;
}

bool ModuleUI::Update() {
    float dt = Application::GetInstance().time->GetRealDeltaTime();
    Application& app = Application::GetInstance();

    if (app.input->GetKey(SDL_SCANCODE_F1) == KEY_DOWN) {
        auto configWin = app.editor->GetConfigurationWindow(); 
        if(configWin) configWin->SetOpen(!configWin->IsOpen());
    }

    if (currentState == UIState::MAIN_MENU) {
        app.camera->SetUseEditorCamera(false);
        if (app.input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN && strlen(nameBuffer) > 0)
            currentState = UIState::FADING;
    } 
    else if (currentState == UIState::FADING) {
        fadeAlpha += dt * fadeSpeed;
        if (fadeAlpha >= 1.0f) {
            fadeAlpha = 1.0f;
            currentState = UIState::GAME_HUD;
            app.Play(); 
            app.camera->SetUseEditorCamera(true);
            SaveConfig(); 
        }
    }
    return true;
}

bool ModuleUI::PostUpdate() {
    if (currentState == UIState::MAIN_MENU) DrawMainMenu();
    if (currentState == UIState::FADING || (currentState == UIState::GAME_HUD && fadeAlpha > 0.0f)) DrawFade();
    return true;
}

void ModuleUI::DrawMainMenu() {
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetForegroundDrawList(); 
    if (backgroundTextureID != 0) drawList->AddImage((ImTextureID)(uintptr_t)backgroundTextureID, ImVec2(0, 0), io.DisplaySize);
    else drawList->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(30, 30, 30, 255));

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Login", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("BIENVENIDO AL MOTOR");
    ImGui::InputText("Nombre", nameBuffer, 64);
    if (ImGui::Button("START", ImVec2(ImGui::GetContentRegionAvail().x, 40))) {
        if (strlen(nameBuffer) > 0) currentState = UIState::FADING;
    }
    ImGui::End();
}

void ModuleUI::DrawFade() {
    ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(0, 0), ImGui::GetIO().DisplaySize, IM_COL32(0, 0, 0, (int)(fadeAlpha * 255)));
}

void ModuleUI::SaveConfig() {
    userName = nameBuffer;
    nlohmann::json j;
    j["userName"] = userName;
    std::ofstream o("UIConfig.json");
    if (o.is_open()) o << j.dump(4);
}

void ModuleUI::LoadConfig() {
    std::ifstream i("UIConfig.json");
    if (i.is_open()) {
        nlohmann::json j; i >> j;
        userName = j.value("userName", "");
        strcpy_s(nameBuffer, 64, userName.c_str());
    }
}