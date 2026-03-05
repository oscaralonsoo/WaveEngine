#include "ModuleCamera.h"
#include "GameWindow.h"
#include "ComponentCamera.h"
#include "CameraLens.h"
#include "Application.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentCanvas.h"
#include "Input.h"
#include <imgui.h>
#include "NsGui/InputEnums.h"
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_mouse.h>

static Noesis::Key SDLScancodeToNoesisKey(SDL_Scancode sc)
{
    switch (sc)
    {
        // Navigation / UI keys
    case SDL_SCANCODE_RETURN:       return Noesis::Key_Return;
    case SDL_SCANCODE_RETURN2:      return Noesis::Key_Return;
    case SDL_SCANCODE_KP_ENTER:     return Noesis::Key_Return;
    case SDL_SCANCODE_ESCAPE:       return Noesis::Key_Escape;
    case SDL_SCANCODE_SPACE:        return Noesis::Key_Space;
    case SDL_SCANCODE_BACKSPACE:    return Noesis::Key_Back;
    case SDL_SCANCODE_DELETE:       return Noesis::Key_Delete;
    case SDL_SCANCODE_TAB:          return Noesis::Key_Tab;

        // Arrow keys
    case SDL_SCANCODE_UP:           return Noesis::Key_Up;
    case SDL_SCANCODE_DOWN:         return Noesis::Key_Down;
    case SDL_SCANCODE_LEFT:         return Noesis::Key_Left;
    case SDL_SCANCODE_RIGHT:        return Noesis::Key_Right;

        // Page / Home / End
    case SDL_SCANCODE_HOME:         return Noesis::Key_Home;
    case SDL_SCANCODE_END:          return Noesis::Key_End;
    case SDL_SCANCODE_PAGEUP:       return Noesis::Key_PageUp;
    case SDL_SCANCODE_PAGEDOWN:     return Noesis::Key_PageDown;

        // Modifiers
    case SDL_SCANCODE_LSHIFT:       return Noesis::Key_LeftShift;
    case SDL_SCANCODE_RSHIFT:       return Noesis::Key_RightShift;
    case SDL_SCANCODE_LCTRL:        return Noesis::Key_LeftCtrl;
    case SDL_SCANCODE_RCTRL:        return Noesis::Key_RightCtrl;
    case SDL_SCANCODE_LALT:         return Noesis::Key_LeftAlt;
    case SDL_SCANCODE_RALT:         return Noesis::Key_RightAlt;

        // Alphabet
    case SDL_SCANCODE_A: return Noesis::Key_A; case SDL_SCANCODE_B: return Noesis::Key_B;
    case SDL_SCANCODE_C: return Noesis::Key_C; case SDL_SCANCODE_D: return Noesis::Key_D;
    case SDL_SCANCODE_E: return Noesis::Key_E; case SDL_SCANCODE_F: return Noesis::Key_F;
    case SDL_SCANCODE_G: return Noesis::Key_G; case SDL_SCANCODE_H: return Noesis::Key_H;
    case SDL_SCANCODE_I: return Noesis::Key_I; case SDL_SCANCODE_J: return Noesis::Key_J;
    case SDL_SCANCODE_K: return Noesis::Key_K; case SDL_SCANCODE_L: return Noesis::Key_L;
    case SDL_SCANCODE_M: return Noesis::Key_M; case SDL_SCANCODE_N: return Noesis::Key_N;
    case SDL_SCANCODE_O: return Noesis::Key_O; case SDL_SCANCODE_P: return Noesis::Key_P;
    case SDL_SCANCODE_Q: return Noesis::Key_Q; case SDL_SCANCODE_R: return Noesis::Key_R;
    case SDL_SCANCODE_S: return Noesis::Key_S; case SDL_SCANCODE_T: return Noesis::Key_T;
    case SDL_SCANCODE_U: return Noesis::Key_U; case SDL_SCANCODE_V: return Noesis::Key_V;
    case SDL_SCANCODE_W: return Noesis::Key_W; case SDL_SCANCODE_X: return Noesis::Key_X;
    case SDL_SCANCODE_Y: return Noesis::Key_Y; case SDL_SCANCODE_Z: return Noesis::Key_Z;

        // Numbers
    case SDL_SCANCODE_0: return Noesis::Key_D0; case SDL_SCANCODE_1: return Noesis::Key_D1;
    case SDL_SCANCODE_2: return Noesis::Key_D2; case SDL_SCANCODE_3: return Noesis::Key_D3;
    case SDL_SCANCODE_4: return Noesis::Key_D4; case SDL_SCANCODE_5: return Noesis::Key_D5;
    case SDL_SCANCODE_6: return Noesis::Key_D6; case SDL_SCANCODE_7: return Noesis::Key_D7;
    case SDL_SCANCODE_8: return Noesis::Key_D8; case SDL_SCANCODE_9: return Noesis::Key_D9;

        // Function keys
    case SDL_SCANCODE_F1:  return Noesis::Key_F1;  case SDL_SCANCODE_F2:  return Noesis::Key_F2;
    case SDL_SCANCODE_F3:  return Noesis::Key_F3;  case SDL_SCANCODE_F4:  return Noesis::Key_F4;
    case SDL_SCANCODE_F5:  return Noesis::Key_F5;  case SDL_SCANCODE_F6:  return Noesis::Key_F6;
    case SDL_SCANCODE_F7:  return Noesis::Key_F7;  case SDL_SCANCODE_F8:  return Noesis::Key_F8;
    case SDL_SCANCODE_F9:  return Noesis::Key_F9;  case SDL_SCANCODE_F10: return Noesis::Key_F10;
    case SDL_SCANCODE_F11: return Noesis::Key_F11; case SDL_SCANCODE_F12: return Noesis::Key_F12;

    default: return Noesis::Key_None;
    }
}

// SDL mouse button index
static Noesis::MouseButton SDLMouseButtonToNoesis(Uint8 sdlBtn)
{
    switch (sdlBtn)
    {
    case SDL_BUTTON_LEFT:   return Noesis::MouseButton_Left;
    case SDL_BUTTON_RIGHT:  return Noesis::MouseButton_Right;
    case SDL_BUTTON_MIDDLE: return Noesis::MouseButton_Middle;
    case SDL_BUTTON_X1:     return Noesis::MouseButton_XButton1;
    case SDL_BUTTON_X2:     return Noesis::MouseButton_XButton2;
    default:                return Noesis::MouseButton_Left;
    }
}

static Noesis::Key GamepadButtonToNoesisKey(GamepadButton btn)
{
    switch (btn)
    {
    case GP_SOUTH:          return Noesis::Key_GamepadAccept;   // A / Cross  → Accept / click
    case GP_EAST:           return Noesis::Key_GamepadCancel;   // B / Circle → Cancel / back
    case GP_WEST:           return Noesis::Key_GamepadView;     // X / Square → Context/view
    case GP_NORTH:          return Noesis::Key_GamepadMenu;     // Y / Triangle → Menu
    case GP_START:          return Noesis::Key_GamepadMenu;
    case GP_BACK:           return Noesis::Key_GamepadView;
    case GP_DPAD_UP:        return Noesis::Key_GamepadUp;
    case GP_DPAD_DOWN:      return Noesis::Key_GamepadDown;
    case GP_DPAD_LEFT:      return Noesis::Key_GamepadLeft;
    case GP_DPAD_RIGHT:     return Noesis::Key_GamepadRight;
    case GP_LEFT_SHOULDER:  return Noesis::Key_GamepadPageLeft;
    case GP_RIGHT_SHOULDER: return Noesis::Key_GamepadPageRight;
    default:                return Noesis::Key_None;
    }
}

static std::vector<ComponentCanvas*> GetActiveCanvases()
{
    std::vector<ComponentCanvas*> result;
    if (!Application::GetInstance().scene) return result;

    GameObject* root = Application::GetInstance().scene->GetRoot();
    if (!root) return result;

    std::vector<Component*> comps;
    root->GetComponentsInChildren(ComponentType::CANVAS, comps);

    for (Component* c : comps)
    {
        ComponentCanvas* canvas = static_cast<ComponentCanvas*>(c);
        if (canvas && canvas->IsActive() && canvas->GetOwner()->IsActive())
            result.push_back(canvas);
    }
    return result;
}

GameWindow::GameWindow()
    : EditorWindow("Game")
{
}

void GameWindow::DispatchInputToCanvases()
{
    bool isPlaying = (Application::GetInstance().GetPlayState() == Application::PlayState::PLAYING);
    if (!isPlaying) return;

    auto canvases = GetActiveCanvases();
    if (canvases.empty()) return;

    Input* input = Application::GetInstance().input.get();
    if (!input) return;

    //Mouse
    {
        int mx = input->GetMouseX() - static_cast<int>(gameViewportPos.x);
        int my = input->GetMouseY() - static_cast<int>(gameViewportPos.y);

        for (ComponentCanvas* canvas : canvases)
        {
            canvas->OnMouseMove(mx, my);
        }

        for (int btn = 1; btn <= NUM_MOUSE_BUTTONS; ++btn)
        {
            KeyState state = input->GetMouseButtonDown(btn);
            Noesis::MouseButton nsBtn = SDLMouseButtonToNoesis(static_cast<Uint8>(btn));

            if (state == KEY_DOWN)
                for (ComponentCanvas* canvas : canvases)
                    canvas->OnMouseButtonDown(mx, my, nsBtn);
            else if (state == KEY_UP)
                for (ComponentCanvas* canvas : canvases)
                    canvas->OnMouseButtonUp(mx, my, nsBtn);
        }

        // Mouse wheel
        float wheel = input->GetMouseWheelY();
        if (wheel != 0.0f)
        {
            int delta = static_cast<int>(wheel * 120);
            for (ComponentCanvas* canvas : canvases)
                canvas->OnMouseWheel(mx, my, delta);
        }
    }

    //Keyboard 
    for (int sc = 0; sc < MAX_KEYS; ++sc)
    {
        KeyState ks = input->GetKey(sc);
        if (ks == KEY_IDLE || ks == KEY_REPEAT) continue;

        Noesis::Key nsKey = SDLScancodeToNoesisKey(static_cast<SDL_Scancode>(sc));
        if (nsKey == Noesis::Key_None) continue;

        if (ks == KEY_DOWN)
            for (ComponentCanvas* canvas : canvases)
                canvas->OnGamepadButtonDown(nsKey);
        else if (ks == KEY_UP)
            for (ComponentCanvas* canvas : canvases)
                canvas->OnGamepadButtonUp(nsKey);
    }

    //Gamepad
    if (input->HasGamepad())
    {
        // Buttons
        const GamepadButton allButtons[] = {
            GP_SOUTH, GP_EAST, GP_WEST, GP_NORTH,
            GP_BACK, GP_START,
            GP_LEFT_SHOULDER, GP_RIGHT_SHOULDER,
            GP_DPAD_UP, GP_DPAD_DOWN, GP_DPAD_LEFT, GP_DPAD_RIGHT
        };

        for (GamepadButton btn : allButtons)
        {
            KeyState ks = input->GetGamepadButton(btn, 0);
            Noesis::Key nsKey = GamepadButtonToNoesisKey(btn);
            if (nsKey == Noesis::Key_None) continue;

            if (ks == KEY_DOWN)
                for (ComponentCanvas* canvas : canvases)
                    canvas->OnGamepadButtonDown(nsKey);
            else if (ks == KEY_UP)
                for (ComponentCanvas* canvas : canvases)
                    canvas->OnGamepadButtonUp(nsKey);
        }

        // Left stick
        glm::vec2 ls = input->GetLeftStick(0);
        for (ComponentCanvas* canvas : canvases)
            canvas->OnGamepadLeftStick(ls.x, ls.y);

        // Right stick
        glm::vec2 rs = input->GetRightStick(0);
        for (ComponentCanvas* canvas : canvases)
            canvas->OnGamepadRightStick(rs.x, rs.y);

        // Triggers
        float lt = input->GetGamepadAxis(GP_AXIS_LEFT_TRIGGER, 0);
        float rt = input->GetGamepadAxis(GP_AXIS_RIGHT_TRIGGER, 0);
        for (ComponentCanvas* canvas : canvases)
            canvas->OnGamepadTrigger(lt, rt);
    }
}

void GameWindow::Draw()
{
    if (!isOpen) return;

    DispatchInputToCanvases();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(name.c_str(), &isOpen);

    isHovered = (ImGui::IsWindowHovered(
        ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows));

    gameViewportPos = ImGui::GetCursorScreenPos();
    gameViewportSize = ImGui::GetContentRegionAvail();

    ComponentCamera* cameraComp = Application::GetInstance().camera->GetMainCamera();

    if (cameraComp && cameraComp->GetLens())
    {
        CameraLens* camera = cameraComp->GetLens();
        if (camera && gameViewportSize.x > 1 && gameViewportSize.y > 1)
        {
            if (gameViewportSize.x != camera->textureWidth ||
                gameViewportSize.y != camera->textureHeight)
            {
                camera->SetRenderTarget(
                    static_cast<int>(gameViewportSize.x),
                    static_cast<int>(gameViewportSize.y));
                float aspect = gameViewportSize.x / gameViewportSize.y;
                camera->SetPerspective(
                    camera->GetFov(), aspect,
                    camera->GetNearPlane(), camera->GetFarPlane());
            }

            GLuint gameTexture = camera->textureID;
            if (gameTexture != 0)
            {
                ImTextureID texID = (ImTextureID)(uintptr_t)gameTexture;
                ImGui::Image(texID, gameViewportSize,
                    ImVec2(0, 1), ImVec2(1, 0));
            }
        }
    }
    else
    {
        const char* text = "There's no main camera in scene";
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImVec2 windowPos = ImGui::GetWindowPos();
        float  textX = windowPos.x + (gameViewportSize.x - textSize.x) * 0.5f;
        float  textY = windowPos.y + (gameViewportSize.y - textSize.y) * 0.5f;
        ImGui::SetCursorScreenPos(ImVec2(textX, textY));
        ImGui::Text("%s", text);
    }

    //Draw Canvas overlays
    if (Application::GetInstance().scene)
    {
        GameObject* root = Application::GetInstance().scene->GetRoot();
        if (root)
        {
            std::vector<Component*> canvasComps;
            root->GetComponentsInChildren(ComponentType::CANVAS, canvasComps);

            for (Component* comp : canvasComps)
            {
                ComponentCanvas* canvas = static_cast<ComponentCanvas*>(comp);
                if (!canvas || !canvas->IsActive() || !canvas->GetOwner()->IsActive())
                    continue;

                if (Application::GetInstance().GetPlayState() ==
                    Application::PlayState::EDITING)
                {
                    canvas->Update();
                }

                canvas->Resize(
                    static_cast<int>(gameViewportSize.x),
                    static_cast<int>(gameViewportSize.y));
                canvas->RenderToTexture();

                ImGui::SetCursorScreenPos(gameViewportPos);
                ImGui::Image(
                    (ImTextureID)(uintptr_t)canvas->GetTextureID(),
                    gameViewportSize,
                    ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f),
                    ImVec4(1.0f, 1.0f, 1.0f, canvas->GetOpacity()),
                    ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}