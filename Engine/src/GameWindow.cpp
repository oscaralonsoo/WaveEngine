#include "ModuleCamera.h"
#include "GameWindow.h"
#include "ComponentCamera.h"
#include "CameraLens.h"
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

    ComponentCamera* cameraComp = Application::GetInstance().camera->GetMainCamera();
   
    if (cameraComp && cameraComp->GetLens())
    {
        CameraLens* camera = cameraComp->GetLens();

        if (camera && gameViewportSize.x > 1 && gameViewportSize.y > 1)
        {
            if (gameViewportSize.x != camera->textureWidth || gameViewportSize.y != camera->textureHeight)
            {
                camera->SetRenderTarget((int)gameViewportSize.x, (int)gameViewportSize.y);

                float aspect = gameViewportSize.x / gameViewportSize.y;
                camera->SetPerspective(camera->GetFov(), aspect, camera->GetNearPlane(), camera->GetFarPlane());
            }

            GLuint gameTexture = camera->textureID;
            if (gameTexture != 0)
            {
                ImTextureID texID = (ImTextureID)(uintptr_t)gameTexture;
                ImGui::Image(texID, gameViewportSize, ImVec2(0, 1), ImVec2(1, 0));
            }
        }
    }
    else
    {
        const char* text = "There's no main camera in scene";
        ImVec2 textSize = ImGui::CalcTextSize(text);

        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = gameViewportSize;

        float textX = windowPos.x + (windowSize.x - textSize.x) * 0.5f;
        float textY = windowPos.y + (windowSize.y - textSize.y) * 0.5f;

        ImGui::SetCursorScreenPos(ImVec2(textX, textY));

        ImGui::Text(text);
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
