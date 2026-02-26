#include "Input.h"
#include "Application.h"
/*
#include <iostream>
#ifndef WAVE_GAME
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include <ImGuizmo.h>
#endif
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include <limits>
#include <SDL3/SDL_scancode.h>
*/
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

#ifndef WAVE_GAME
bool IsMouseOverSceneWindow()
{
	ModuleEditor* editor = Application::GetInstance().editor.get();
	if (!editor) return false;

	return editor->IsMouseOverScene();
}
#endif

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
		/*
		#ifndef WAVE_GAME
				ImGui_ImplSDL3_ProcessEvent(&event);
		#endif
		*/
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
			/*
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
				{
					mouseButtons[event.button.button - 1] = KEY_DOWN;
					break;
				}

		#ifndef WAVE_GAME
					ComponentCamera* camera = Application::GetInstance().camera->GetEditorCamera();
					if (!camera) break;

					bool overSceneWindow = IsMouseOverSceneWindow();
					if (!overSceneWindow)
					{
						break;
					}

					float mouseXf, mouseYf;
					SDL_GetMouseState(&mouseXf, &mouseYf);
					int scale = Application::GetInstance().window.get()->GetScale();
					mouseXf /= scale;
					mouseYf /= scale;

					ImGuiIO& io = ImGui::GetIO();
					if (io.WantCaptureMouse && !overSceneWindow)
						break;

					if (!overSceneWindow)
						break;

					if (event.button.button == SDL_BUTTON_RIGHT)
					{
						camera->ResetMouseInput();
						camera->HandleMouseInput(mouseXf, mouseYf);
					}
					else if (event.button.button == SDL_BUTTON_LEFT)
					{
						// Left button over Scene: reset orbit and initialize orbit input
						camera->ResetOrbitInput();
						camera->HandleOrbitInput(mouseXf, mouseYf);

						// Object selection: only if ALT is not pressed
						if (!keys[SDL_SCANCODE_LALT] && !keys[SDL_SCANCODE_RALT] && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
						{
							ImVec2 scenePos = Application::GetInstance().editor->sceneViewportPos;
							ImVec2 sceneSize = Application::GetInstance().editor->sceneViewportSize;

							float relativeX = mouseXf - scenePos.x;
							float relativeY = mouseYf - scenePos.y;

							if (relativeX < 0 || relativeX > sceneSize.x ||
								relativeY < 0 || relativeY > sceneSize.y)
							{
								break;
							}

							glm::vec3 rayOrigin = camera->GetPosition();
							glm::vec3 rayDir = camera->ScreenToWorldRay(
								static_cast<int>(relativeX),
								static_cast<int>(relativeY),
								static_cast<int>(sceneSize.x),
								static_cast<int>(sceneSize.y)
							);

							GameObject* root = Application::GetInstance().scene->GetRoot();
							float minDist = std::numeric_limits<float>::max();
							GameObject* clicked = FindClosestObjectToRayOptimized(root, rayOrigin, rayDir, minDist);

							bool shiftPressed = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

							if (clicked)
							{
								if (shiftPressed)
								{
									Application::GetInstance().selectionManager->ToggleSelection(clicked);
								}
								else
								{
									Application::GetInstance().selectionManager->SetSelectedObject(clicked);
								}
							}
							else
							{
								if (!shiftPressed)
								{
									Application::GetInstance().selectionManager->ClearSelection();
								}
							}
						}
						else
						{
							camera->ResetOrbitInput();
							camera->HandleOrbitInput(mouseXf, mouseYf);
						}
					}
					else if (event.button.button == SDL_BUTTON_MIDDLE)
					{
						camera->ResetPanInput();
					}
		#endif
		*/
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
			/*		int scale = Application::GetInstance().window.get()->GetScale();
						mouseMotionX = static_cast<int>(event.motion.xrel / scale);
						mouseMotionY = static_cast<int>(event.motion.yrel / scale);
						mouseX = static_cast<int>(event.motion.x / scale);
						mouseY = static_cast<int>(event.motion.y / scale);

			#ifndef WAVE_GAME
						float mouseXf = static_cast<float>(event.motion.x) / static_cast<float>(scale);
						float mouseYf = static_cast<float>(event.motion.y) / static_cast<float>(scale);

						bool overSceneWindow = IsMouseOverSceneWindow();
						bool isDragging = (mouseButtons[SDL_BUTTON_LEFT - 1] == KEY_REPEAT ||
							mouseButtons[SDL_BUTTON_LEFT - 1] == KEY_DOWN ||
							mouseButtons[SDL_BUTTON_MIDDLE - 1] == KEY_REPEAT ||
							mouseButtons[SDL_BUTTON_MIDDLE - 1] == KEY_DOWN ||
							mouseButtons[SDL_BUTTON_RIGHT - 1] == KEY_REPEAT ||
							mouseButtons[SDL_BUTTON_RIGHT - 1] == KEY_DOWN);

						if (overSceneWindow || isDragging)
						{
							ComponentCamera* camera = Application::GetInstance().camera->GetEditorCamera();
							if (!camera) break;

							if ((keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT]) &&
								(mouseButtons[SDL_BUTTON_LEFT - 1] == KEY_REPEAT || mouseButtons[SDL_BUTTON_LEFT - 1] == KEY_DOWN))
							{
								camera->HandleOrbitInput(mouseXf, mouseYf);
							}
							else if (mouseButtons[SDL_BUTTON_MIDDLE - 1] == KEY_REPEAT || mouseButtons[SDL_BUTTON_MIDDLE - 1] == KEY_DOWN)
							{
								camera->HandlePanInput(static_cast<float>(mouseMotionX), static_cast<float>(mouseMotionY));
							}
							else if (mouseButtons[SDL_BUTTON_RIGHT - 1] == KEY_REPEAT || mouseButtons[SDL_BUTTON_RIGHT - 1] == KEY_DOWN)
							{
								camera->HandleMouseInput(mouseXf, mouseYf);
							}
						}
			#endif

						break;
			*/
			mouseMotionX = event.motion.xrel / scale;
			mouseMotionY = event.motion.yrel / scale;
			mouseX = event.motion.x / scale;
			mouseY = event.motion.y / scale;
		}
		break;
		case SDL_EVENT_MOUSE_WHEEL:
		{
			/*
#ifndef WAVE_GAME
			float mouseXf, mouseYf;
			SDL_GetMouseState(&mouseXf, &mouseYf);
			int scale = Application::GetInstance().window.get()->GetScale();
			mouseXf /= scale;
			mouseYf /= scale;

			if (IsMouseOverSceneWindow())
			{
				ComponentCamera* camera = Application::GetInstance().camera->GetEditorCamera();
				if (camera)
				{
					camera->HandleScrollInput(static_cast<float>(event.wheel.y));
				}
			}
#endif
			break;
		}
		}
	}

#ifndef WAVE_GAME
	// Camera movement with WASD
	ComponentCamera* camera = Application::GetInstance().camera->GetEditorCamera();
	if (!camera) return true;
	GameObject* cameraGO = camera->owner;
	Transform* cameraTransform = static_cast<Transform*>(cameraGO->GetComponent(ComponentType::TRANSFORM));
	if (!cameraTransform) return true;

	const float cameraBaseSpeed = camera->GetMovementSpeed();
	float speedMultiplier = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] ? 2.0f : 1.0f;
	float cameraSpeed = cameraBaseSpeed * speedMultiplier * Application::GetInstance().time->GetDeltaTime();

	Uint32 mouseState = SDL_GetMouseState(NULL, NULL);
	bool rightMousePressed = (mouseState & SDL_BUTTON_RMASK) != 0;

	if (rightMousePressed && (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_D]))
	{
		glm::vec3 cameraPos = camera->GetPosition();
		glm::vec3 cameraFront = camera->GetFront();
		glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, camera->GetUp()));

		if (keys[SDL_SCANCODE_W])
			cameraPos += cameraSpeed * cameraFront;
		if (keys[SDL_SCANCODE_S])
			cameraPos -= cameraSpeed * cameraFront;
		if (keys[SDL_SCANCODE_A])
			cameraPos -= cameraRight * cameraSpeed;
		if (keys[SDL_SCANCODE_D])
			cameraPos += cameraRight * cameraSpeed;

		cameraTransform->SetPosition(cameraPos);
	}

	glm::vec3 cameraUp = camera->GetUp();
	if (keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL])
	{
		glm::vec3 cameraPos = camera->GetPosition();

		if (keys[SDL_SCANCODE_SPACE])
			cameraPos += cameraUp * cameraSpeed;
		if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL])
			cameraPos -= cameraUp * cameraSpeed;

		cameraTransform->SetPosition(cameraPos);
	}

	if (keyboard[SDL_SCANCODE_F] == KEY_DOWN)
	{
		if (Application::GetInstance().selectionManager->HasSelection())
		*/
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
		/*
		#endif
		*/
		else
		{
			for (int i = 0; i < MAX_GAMEPAD_BUTTONS; ++i)
				gamepadButtons[i] = KEY_IDLE;
		}

		return true;
	}
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