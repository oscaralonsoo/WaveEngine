#include "Input.h"
#include "Application.h"
#include "Globals.h"
#include "ModuleEvents.h"

#include "Log.h"
#include "glm/glm.hpp"

Input::Input() : Module()
{
	name = "input";

	keyboard = new KeyState[MAX_KEYS];
	memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
	memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
}

Input::~Input()
{

}

bool Input::Awake()
{
	bool ret = true;
	SDL_Init(0);

	if (SDL_InitSubSystem(SDL_INIT_EVENTS | SDL_INIT_GAMEPAD) < 0)
	{
		LOG_DEBUG("SDL_EVENTS could not initialize! SDL_Error: %s\n", SDL_GetError());
		return false;
	}

	int count = 0;
	SDL_JoystickID* joysticks = SDL_GetJoysticks(&count);

	if (joysticks && count > 0)
	{
		for (int i = 0; i < count; ++i)
		{
			SDL_JoystickID joystick_id = joysticks[i];

			if (SDL_IsGamepad(joystick_id))
			{
				controller = SDL_OpenGamepad(joystick_id);
				if (controller) {
					const char* name = SDL_GetGamepadName(controller);
					LOG_DEBUG("Gamepad connected: %s", name ? name : "Unknown");
					break;
				}
			}
		}
	}


	if (joysticks) {
		SDL_free(joysticks);
	}

	scale = 1.0f;

	return ret;
}

bool Input::Start()
{
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

		}
	};

	if (controller != nullptr)
	{
		for (int i = 0; i < MAX_GAMEPAD_BUTTONS; ++i)
		{
			bool isPressed = SDL_GetGamepadButton(controller, static_cast<SDL_GamepadButton>(i));

			switch (gamepadButtons[i])
			{
			case KEY_IDLE:
				if (isPressed)
					gamepadButtons[i] = KEY_DOWN;
				break;
			case KEY_DOWN:
				gamepadButtons[i] = isPressed ? KEY_REPEAT : KEY_UP;
				break;
			case KEY_REPEAT:
				if (!isPressed)
					gamepadButtons[i] = KEY_UP;
				break;
			case KEY_UP:
				gamepadButtons[i] = isPressed ? KEY_DOWN : KEY_IDLE;
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < MAX_GAMEPAD_BUTTONS; ++i)
			gamepadButtons[i] = KEY_IDLE;
	}

	return true;
}

bool Input::CleanUp()
{
	LOG_DEBUG("Quitting SDL event subsystem");
	SDL_QuitSubSystem(SDL_INIT_EVENTS);
	delete[] keyboard;

	if (controller != nullptr) {
		SDL_CloseGamepad(controller);
		controller = nullptr;
	}
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

KeyState Input::GetGamepadButton(SDL_GamepadButton button) const
{
	return gamepadButtons[button];
}

float Input::GetMouseWheelY()
{
	return mouseWheelY;
}