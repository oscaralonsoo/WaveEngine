#pragma once

#include "Module.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_rect.h"
#include <vector>
#include <functional>
#include <string>
#include "glm/glm.hpp"

#define MAX_KEYS SDL_SCANCODE_COUNT
#define NUM_MOUSE_BUTTONS 5

// Normalized dead zone 0 to 1, set on 0.15f by default
#define GAMEPAD_DEAD_ZONE 0.15f

// Gamepad buttons mapped to SDL3 SDL_GamepadButton values
enum GamepadButton
{
	GP_SOUTH = SDL_GAMEPAD_BUTTON_SOUTH,
	GP_EAST = SDL_GAMEPAD_BUTTON_EAST,
	GP_WEST = SDL_GAMEPAD_BUTTON_WEST,
	GP_NORTH = SDL_GAMEPAD_BUTTON_NORTH,
	GP_BACK = SDL_GAMEPAD_BUTTON_BACK,
	GP_GUIDE = SDL_GAMEPAD_BUTTON_GUIDE,
	GP_START = SDL_GAMEPAD_BUTTON_START,
	GP_LEFT_STICK = SDL_GAMEPAD_BUTTON_LEFT_STICK,
	GP_RIGHT_STICK = SDL_GAMEPAD_BUTTON_RIGHT_STICK,
	GP_LEFT_SHOULDER = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
	GP_RIGHT_SHOULDER = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
	GP_DPAD_UP = SDL_GAMEPAD_BUTTON_DPAD_UP,
	GP_DPAD_DOWN = SDL_GAMEPAD_BUTTON_DPAD_DOWN,
	GP_DPAD_LEFT = SDL_GAMEPAD_BUTTON_DPAD_LEFT,
	GP_DPAD_RIGHT = SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
	GP_BUTTON_COUNT = SDL_GAMEPAD_BUTTON_COUNT
};

enum GamepadAxis
{
	GP_AXIS_LEFT_X = SDL_GAMEPAD_AXIS_LEFTX,
	GP_AXIS_LEFT_Y = SDL_GAMEPAD_AXIS_LEFTY,
	GP_AXIS_RIGHT_X = SDL_GAMEPAD_AXIS_RIGHTX,
	GP_AXIS_RIGHT_Y = SDL_GAMEPAD_AXIS_RIGHTY,
	GP_AXIS_LEFT_TRIGGER = SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
	GP_AXIS_RIGHT_TRIGGER = SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
	GP_AXIS_COUNT = SDL_GAMEPAD_AXIS_COUNT
};

enum EventWindow
{
	WE_QUIT = 0,
	WE_HIDE = 1,
	WE_SHOW = 2,
	WE_COUNT
};

enum KeyState
{
	KEY_IDLE = 0,
	KEY_DOWN,
	KEY_REPEAT,
	KEY_UP
};

class Input : public Module
{
public:
	Input();
	~Input();

	bool Awake()     override;
	bool Start()     override;
	bool PreUpdate() override;
	bool CleanUp()   override;

	// Keyboard
	KeyState GetKey(int id) const { return keyboard[id]; }

	// Mouse
	KeyState  GetMouseButtonDown(int id) const { return mouseButtons[id - 1]; }
	bool      GetWindowEvent(EventWindow ev);
	glm::vec2 GetMousePosition();
	glm::vec2 GetMouseMotion();
	float     GetMouseWheelY();

	int GetMouseX() const { return mouseX; }
	int GetMouseY() const { return mouseY; }

	// Gamepad
	bool      HasGamepad()      const { return !gamepads.empty(); }
	int       GetGamepadCount() const { return static_cast<int>(gamepads.size()); }

	KeyState  GetGamepadButton(GamepadButton btn, int gamepadIndex = 0)       const;
	bool      IsGamepadButtonPressed(GamepadButton btn, int gamepadIndex = 0) const;
	bool      IsGamepadButtonDown(GamepadButton btn, int gamepadIndex = 0)    const;
	bool      IsGamepadButtonUp(GamepadButton btn, int gamepadIndex = 0)      const;
	float     GetGamepadAxis(GamepadAxis axis, int gamepadIndex = 0)          const;
	glm::vec2 GetLeftStick(int gamepadIndex = 0)                              const;
	glm::vec2 GetRightStick(int gamepadIndex = 0)                             const;

	// Rumble, Vibration feedback
	// lowFreq: Left Engine units from 0.0 to 1.0. The left Engine in Gamepads is used for STRONG VIBRATIONS, EXPLOSIONS, CRASHES AND SHOTS.
	// highFreq: Right Engine units from 0.0 to 1.0. The Right Engine in Gamepads is used for LOW VIBRATIONS, EXPLOSIONS, CRASHES AND SHOTS.
	// This is because the left engine has biggers counterweights and the right engine smallers but they are quicker.
	// durationMs: duration time of the rumble in miliseconds
	void RumbleGamepad(float lowFreq, float highFreq, Uint32 durationMs, int gamepadIndex = 0);
	void StopRumble(int gamepadIndex = 0);

	using InputListener = std::function<void(SDL_Event*)>;

private:
	// Keyboard and mouse
	bool windowEvents[WE_COUNT];
	KeyState* keyboard;
	KeyState  mouseButtons[NUM_MOUSE_BUTTONS];
	int   mouseMotionX = 0;
	int   mouseMotionY = 0;
	int   mouseX = 0;
	int   mouseY = 0;
	int   scale = 1;
	float mouseWheelY = 0.0f;

	// Gamepad
	struct GamepadState
	{
		SDL_Gamepad* handle = nullptr;
		SDL_JoystickID instanceId = 0;
		std::string    name;
		// saved in OnGamepadAdded
		bool           connected = false;
		KeyState       buttons[GP_BUTTON_COUNT];
		float          axes[GP_AXIS_COUNT];

		GamepadState()
		{
			for (int i = 0; i < GP_BUTTON_COUNT; ++i) buttons[i] = KEY_IDLE;
			for (int i = 0; i < GP_AXIS_COUNT; ++i) axes[i] = 0.0f;
		}

		// Forces all states to released in one frame to prevent any Key stays in REPEAT mode
		void FlushToIdle()
		{
			for (int i = 0; i < GP_BUTTON_COUNT; ++i)
				if (buttons[i] == KEY_DOWN || buttons[i] == KEY_REPEAT)
					buttons[i] = KEY_UP;
			for (int i = 0; i < GP_AXIS_COUNT; ++i)
				axes[i] = 0.0f;
		}
	};

	std::vector<GamepadState> gamepads;

	int   FindGamepadIndex(SDL_JoystickID id) const;
	void  OnGamepadAdded(SDL_JoystickID id);
	void  OnGamepadRemoved(SDL_JoystickID id);
	void  PurgeInvalidGamepads();
	void  UpdateGamepadStates();
	float ApplyDeadZone(float value) const;
};