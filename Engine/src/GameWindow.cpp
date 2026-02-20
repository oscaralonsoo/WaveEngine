#include "GameWindow.h"
#include "Application.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentCanvas.h"
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

    // Get the game texture from the renderer
    GLuint gameTexture = Application::GetInstance().renderer->GetGameTexture();
    if (gameTexture != 0 && gameViewportSize.x > 0 && gameViewportSize.y > 0)
    {
        ImTextureID texID = (ImTextureID)(uintptr_t)gameTexture;
        ImGui::Image(texID, gameViewportSize, ImVec2(0, 1), ImVec2(1, 0));
    }
    else
    {
        ImGui::InvisibleButton("GameView", gameViewportSize);
    }

    // Draw Canvases
    if (Application::GetInstance().scene)
    {
        GameObject* root = Application::GetInstance().scene->GetRoot();
        if (root)
        {
            std::vector<Component*> canvases;
            root->GetComponentsInChildren(ComponentType::CANVAS, canvases);

            for (Component* comp : canvases)
            {
                ComponentCanvas* canvas = static_cast<ComponentCanvas*>(comp);
                if (canvas && canvas->IsActive() && canvas->GetOwner()->IsActive())
                {
                    if (Application::GetInstance().GetPlayState() == Application::PlayState::EDITING)
                    {
                        canvas->Update();
                    }

                    canvas->Resize((int)gameViewportSize.x, (int)gameViewportSize.y);
                    canvas->RenderToTexture();
                    ImGui::SetCursorScreenPos(gameViewportPos);
                    ImGui::Image((ImTextureID)(uintptr_t)canvas->GetTextureID(), gameViewportSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f), ImVec4(1.0f, 1.0f, 1.0f, canvas->GetOpacity()), ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                }
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
