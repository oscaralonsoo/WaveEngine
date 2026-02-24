#include "GameWindow.h"
#include "Application.h"
#include <imgui.h>

GameWindow::GameWindow()
    : EditorWindow("Game")
{
}

void GameWindow::Draw()
{
    if (!isOpen) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(name.c_str(), &isOpen);

    isHovered = (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows));

    gameViewportPos = ImGui::GetCursorScreenPos();
    gameViewportSize = ImGui::GetContentRegionAvail();

    //// Get the game texture from the renderer
    //GLuint gameTexture = Application::GetInstance().renderer->GetGameTexture();
    //if (gameTexture != 0 && gameViewportSize.x > 0 && gameViewportSize.y > 0)
    //{
    //    ImTextureID texID = (ImTextureID)(uintptr_t)gameTexture;
    //    ImGui::Image(texID, gameViewportSize, ImVec2(0, 1), ImVec2(1, 0));
    //}
    //else
    //{
    //    ImGui::InvisibleButton("GameView", gameViewportSize);
    //}

    ImGui::End();
    ImGui::PopStyleVar();
}
