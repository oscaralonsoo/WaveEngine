#include "Input.h"
#include "Application.h"
#include "Globals.h"
#include "ModuleEvents.h"

#include "Log.h"
#include "glm/glm.hpp"

#include <algorithm>
#include <SDL3/SDL_scancode.h>

Input::Input() : Module()
{
    name = "input";
    keyboard = new KeyState[MAX_KEYS];
    memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
    memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
}

Input::~Input()
{
    // keyboard is now deleted in CleanUp()
}

bool Input::Awake()
{
    return true;
}

bool Input::Start()
{
    if (SDL_InitSubSystem(SDL_INIT_EVENTS) == false)
    {
        LOG_DEBUG("SDL_INIT_EVENTS failed: %s", SDL_GetError());
        return false;
    }

    if (SDL_InitSubSystem(SDL_INIT_GAMEPAD) == false)
    {
        LOG_DEBUG("[Input] WARNING: SDL_INIT_GAMEPAD failed: %s — gamepad support disabled", SDL_GetError());
        // Prevent crash: Keyboard and mouse still work
    }
    else
    {
        // list gamepads already connected before the engine started
        int count = 0;
        SDL_JoystickID* ids = SDL_GetGamepads(&count);
        if (ids)
        {
            for (int i = 0; i < count; ++i)
                OnGamepadAdded(ids[i]);
            SDL_free(ids);
        }
        LOG_DEBUG("[Input] Gamepad subsystem ready. Controllers connected: %d", static_cast<int>(gamepads.size()));
    }

    scale = 1;
    return true;
}

bool Input::PreUpdate()
{
    static SDL_Event event;
    int numKeys;
    const bool* keys = SDL_GetKeyboardState(&numKeys);
    mouseWheelY = 0;

    for (int i = 0; i < MAX_KEYS && i < numKeys; ++i)
    {
        if (keys[i] == 1)
        {
            if (keyboard[i] == KEY_IDLE)
                keyboard[i] = KEY_DOWN;
            else
                keyboard[i] = KEY_REPEAT;
        }
        else
        {
            if (keyboard[i] == KEY_REPEAT || keyboard[i] == KEY_DOWN)
                keyboard[i] = KEY_UP;
            else
                keyboard[i] = KEY_IDLE;
        }
    }

    for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i)
    {
        if (mouseButtons[i] == KEY_DOWN)
            mouseButtons[i] = KEY_REPEAT;

        if (mouseButtons[i] == KEY_UP)
            mouseButtons[i] = KEY_IDLE;
    }

    while (SDL_PollEvent(&event) != 0)
    {
        Application::GetInstance().events->PublishImmediate(Event(Event::Type::EventSDL, &event));

        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            windowEvents[WE_QUIT] = true;
            break;

        case SDL_EVENT_WINDOW_HIDDEN:
        case SDL_EVENT_WINDOW_MINIMIZED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            windowEvents[WE_HIDE] = true;
            break;

        case SDL_EVENT_WINDOW_SHOWN:
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
            break;
        case SDL_EVENT_WINDOW_RESTORED:
            windowEvents[WE_SHOW] = true;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            Application::GetInstance().events->PublishImmediate(Event(Event::Type::WindowResize, event.window.data1, event.window.data2));
            windowEvents[WE_SHOW] = true;
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            mouseButtons[event.button.button - 1] = KEY_DOWN;
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            mouseButtons[event.button.button - 1] = KEY_UP;
            break;

        case SDL_EVENT_MOUSE_MOTION:
        {
            mouseMotionX = event.motion.xrel / scale;
            mouseMotionY = event.motion.yrel / scale;
            mouseX = event.motion.x / scale;
            mouseY = event.motion.y / scale;
        }
        break;

        case SDL_EVENT_MOUSE_WHEEL:
            mouseWheelY = event.wheel.y;
            break;

        case SDL_EVENT_DROP_FILE:
        {
            const char* filePath = event.drop.data;
            LOG_DEBUG("File dropped in window: %s", filePath);

            std::string filePathStr(filePath);
            std::string extension = filePathStr.substr(filePathStr.find_last_of(".") + 1);

            Application::GetInstance().events->PublishImmediate(Event(Event::Type::FileDropped, filePath));
        }
        break;

        // Gamepad hot-plug while running
        case SDL_EVENT_GAMEPAD_ADDED:
            OnGamepadAdded(event.gdevice.which);
            break;

        case SDL_EVENT_GAMEPAD_REMOVED:
            OnGamepadRemoved(event.gdevice.which);
            break;
        }
    }

    UpdateGamepadStates();

    return true;
}

bool Input::CleanUp()
{
    LOG_DEBUG("Quitting SDL event subsystem");

    for (auto& gp : gamepads)
        if (gp.handle) SDL_CloseGamepad(gp.handle);
    gamepads.clear();

    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    SDL_QuitSubSystem(SDL_INIT_EVENTS);

    delete[] keyboard;
    keyboard = nullptr;

    return true;
}

bool Input::GetWindowEvent(EventWindow ev)
{
    return windowEvents[ev];
}

glm::vec2 Input::GetMousePosition()
{
    return glm::vec2(mouseX, mouseY);
}

glm::vec2 Input::GetMouseMotion()
{
    return glm::vec2(mouseMotionX, mouseMotionY);
}

float Input::GetMouseWheelY()
{
    return mouseWheelY;
}

// Check how many gamepads connected and assign index
int Input::FindGamepadIndex(SDL_JoystickID id) const
{
    for (int i = 0; i < static_cast<int>(gamepads.size()); ++i)
        if (gamepads[i].instanceId == id) return i;
    return -1;
}

// Hot-plug gamepad behaviour
void Input::OnGamepadAdded(SDL_JoystickID id)
{
    if (FindGamepadIndex(id) != -1) return;

    SDL_Gamepad* handle = SDL_OpenGamepad(id);
    if (!handle)
    {
        LOG_DEBUG("[Input] WARNING: SDL_OpenGamepad(id=%d) failed: %s", id, SDL_GetError());
        return;
    }

    GamepadState gs;
    gs.handle = handle;
    gs.instanceId = id;
    gs.connected = true;

    const char* rawName = SDL_GetGamepadName(handle);
    gs.name = rawName ? rawName : "Unknown Gamepad";

    gamepads.push_back(gs);
    LOG_DEBUG("[Input] Gamepad connected [%d]: %s", static_cast<int>(gamepads.size()), gs.name.c_str());
}

void Input::OnGamepadRemoved(SDL_JoystickID id)
{
    int idx = FindGamepadIndex(id);
    if (idx == -1) return; // late or duplicate event, ignore and keep the index 1

    GamepadState& gp = gamepads[idx];
    gp.FlushToIdle(); // send scripts one frame of KEY_UP before removal of the gamepad to prevent false gamepads inputs
    gp.connected = false;

    LOG_DEBUG("[Input] Gamepad disconnected: %s (id=%d)", gp.name.c_str(), id);

    if (gp.handle) { SDL_CloseGamepad(gp.handle); gp.handle = nullptr; }

    if (idx != static_cast<int>(gamepads.size()) - 1)
        std::swap(gamepads[idx], gamepads.back());
    gamepads.pop_back();

    LOG_DEBUG("[Input] Gamepads remaining: %d", static_cast<int>(gamepads.size()));
}

void Input::PurgeInvalidGamepads()
{
    for (int i = static_cast<int>(gamepads.size()) - 1; i >= 0; --i)
    {
        GamepadState& gp = gamepads[i];
        bool invalid = !gp.handle || !SDL_GamepadConnected(gp.handle);

        if (invalid)
        {
            LOG_DEBUG("[Input] Purging stale gamepad: %s", gp.name.c_str());
            gp.FlushToIdle();
            if (gp.handle) { SDL_CloseGamepad(gp.handle); gp.handle = nullptr; }
            if (i != static_cast<int>(gamepads.size()) - 1)
                std::swap(gamepads[i], gamepads.back());
            gamepads.pop_back();
        }
    }
}

// Gamepad  dead zone to prevent drifting
float Input::ApplyDeadZone(float value) const
{
    if (value > -GAMEPAD_DEAD_ZONE && value < GAMEPAD_DEAD_ZONE) return 0.0f;
    float sign = value > 0.0f ? 1.0f : -1.0f;
    return sign * (std::abs(value) - GAMEPAD_DEAD_ZONE) / (1.0f - GAMEPAD_DEAD_ZONE);
}

// Gamepad events update
void Input::UpdateGamepadStates()
{
    PurgeInvalidGamepads();

    for (auto& gp : gamepads)
    {
        if (!gp.handle || !gp.connected) continue;

        // Buttons work the same KEY_IDLE/DOWN/REPEAT/UP state event as keyboard
        for (int b = 0; b < GP_BUTTON_COUNT; ++b)
        {
            bool held = SDL_GetGamepadButton(gp.handle, static_cast<SDL_GamepadButton>(b));
            KeyState& s = gp.buttons[b];
            if (held)
                s = (s == KEY_IDLE || s == KEY_UP) ? KEY_DOWN : KEY_REPEAT;
            else
                s = (s == KEY_DOWN || s == KEY_REPEAT) ? KEY_UP : KEY_IDLE;
        }

        // Axes is different because SDL3 returns Sint16 number between -32768 and 32767
        // So its normalized by code to -1 and 1 then apply the dead zone
        for (int a = 0; a < GP_AXIS_COUNT; ++a)
        {
            Sint16 raw = SDL_GetGamepadAxis(gp.handle, static_cast<SDL_GamepadAxis>(a));
            float  norm = (raw < 0) ? (raw / 32768.0f) : (raw / 32767.0f);
            gp.axes[a] = ApplyDeadZone(norm);
        }
    }
}

// Gamepad api call
KeyState Input::GetGamepadButton(GamepadButton btn, int idx) const
{
    if (idx < 0 || idx >= static_cast<int>(gamepads.size())) return KEY_IDLE;
    int b = static_cast<int>(btn);
    if (b < 0 || b >= GP_BUTTON_COUNT) return KEY_IDLE;
    return gamepads[idx].buttons[b];
}

// Gamepad checks
bool Input::IsGamepadButtonPressed(GamepadButton btn, int idx) const
{
    KeyState s = GetGamepadButton(btn, idx);
    return s == KEY_DOWN || s == KEY_REPEAT;
}

bool Input::IsGamepadButtonDown(GamepadButton btn, int idx) const
{
    return GetGamepadButton(btn, idx) == KEY_DOWN;
}

bool Input::IsGamepadButtonUp(GamepadButton btn, int idx) const
{
    return GetGamepadButton(btn, idx) == KEY_UP;
}

float Input::GetGamepadAxis(GamepadAxis axis, int idx) const
{
    if (idx < 0 || idx >= static_cast<int>(gamepads.size())) return 0.0f;
    int a = static_cast<int>(axis);
    if (a < 0 || a >= GP_AXIS_COUNT) return 0.0f;
    return gamepads[idx].axes[a];
}

glm::vec2 Input::GetLeftStick(int idx) const
{
    return { GetGamepadAxis(GP_AXIS_LEFT_X, idx), GetGamepadAxis(GP_AXIS_LEFT_Y, idx) };
}

glm::vec2 Input::GetRightStick(int idx) const
{
    return { GetGamepadAxis(GP_AXIS_RIGHT_X, idx), GetGamepadAxis(GP_AXIS_RIGHT_Y, idx) };
}

// Gamepad rumble vibration
static Uint16 FloatToRumble(float v)
{
    if (v <= 0.0f) return 0;
    if (v >= 1.0f) return 65535;
    return static_cast<Uint16>(v * 65535.0f);
}

void Input::RumbleGamepad(float lowFreq, float highFreq, Uint32 durationMs, int idx)
{
    if (idx < 0 || idx >= static_cast<int>(gamepads.size())) return;
    GamepadState& gp = gamepads[idx];
    if (!gp.handle || !gp.connected) return;

    Uint16 low = FloatToRumble(lowFreq);
    Uint16 high = FloatToRumble(highFreq);

    if (!SDL_RumbleGamepad(gp.handle, low, high, durationMs))
        LOG_DEBUG("[Input] WARNING: Rumble not supported on '%s': %s", gp.name.c_str(), SDL_GetError());
}

void Input::StopRumble(int idx)
{
    if (idx < 0 || idx >= static_cast<int>(gamepads.size())) return;
    GamepadState& gp = gamepads[idx];
    if (!gp.handle || !gp.connected) return;
    SDL_RumbleGamepad(gp.handle, 0, 0, 0);
}