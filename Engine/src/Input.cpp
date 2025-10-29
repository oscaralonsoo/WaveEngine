#include "Input.h"
#include "Window.h"
#include "Application.h"
#include <iostream>
#include "Camera.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"

#define MAX_KEYS 300

Input::Input() : Module(), droppedFile(false), droppedFilePath(""), droppedFileType(DROPPED_NONE)
{
	keyboard = new KeyState[MAX_KEYS];
	// reserve memory
	memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
	memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
}

Input::~Input()
{
	delete[] keyboard;
}

// Called before render is available
bool Input::Awake()
{
	bool ret = true;
	SDL_Init(0);

	if (SDL_InitSubSystem(SDL_INIT_EVENTS) < 0)
	{
		ret = false;
	}

	return ret;
}

// Called before the first frame
bool Input::Start()
{
	return true;
}

// Called each loop iteration
bool Input::PreUpdate()
{
	static SDL_Event event;
	const bool* keys = SDL_GetKeyboardState(NULL);

	// Reset dropped file flag each frame
	droppedFile = false;

	for (int i = 0; i < MAX_KEYS; ++i)
	{
		if (keys[i])
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

	ImGuiIO& io = ImGui::GetIO();
	bool imguiWantCaptureMouse = io.WantCaptureMouse;
	bool imguiWantCaptureKeyboard = io.WantCaptureKeyboard;

	while (SDL_PollEvent(&event))
	{
		// Imgui event
		ImGui_ImplSDL3_ProcessEvent(&event);

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
		case SDL_EVENT_WINDOW_RESTORED:
			windowEvents[WE_SHOW] = true;
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		{
			if (!imguiWantCaptureMouse)
			{
				mouseButtons[event.button.button - 1] = KEY_DOWN;
				Camera* camera = Application::GetInstance().renderer->GetCamera();

				if (event.button.button == SDL_BUTTON_RIGHT)
				{
					camera->ResetMouseInput();
				}
				else if (event.button.button == SDL_BUTTON_LEFT)
				{
					camera->ResetOrbitInput();
				}
				else if (event.button.button == SDL_BUTTON_MIDDLE)
				{
					camera->ResetPanInput();
				}
			}
			break;
		}
		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (!imguiWantCaptureMouse)
			{
				mouseButtons[event.button.button - 1] = KEY_UP;
			}
			break;
		case SDL_EVENT_MOUSE_MOTION:
		{
			if (!imguiWantCaptureMouse)
			{
				int scale = Application::GetInstance().window.get()->GetScale();
				mouseMotionX = static_cast<int>(event.motion.xrel / scale);
				mouseMotionY = static_cast<int>(event.motion.yrel / scale);
				float mouseXf = static_cast<float>(event.motion.x) / static_cast<float>(scale);
				float mouseYf = static_cast<float>(event.motion.y) / static_cast<float>(scale);

				Camera* camera = Application::GetInstance().renderer->GetCamera();

				// Alt + Click Izquierdo - orbit
				if ((keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT]) &&
					(mouseButtons[SDL_BUTTON_LEFT - 1] == KEY_REPEAT || mouseButtons[SDL_BUTTON_LEFT - 1] == KEY_DOWN))
				{
					camera->HandleOrbitInput(mouseXf, mouseYf);
				}
				// Click Medio - Pan
				else if (mouseButtons[SDL_BUTTON_MIDDLE - 1] == KEY_REPEAT || mouseButtons[SDL_BUTTON_MIDDLE - 1] == KEY_DOWN)
				{
					camera->HandlePanInput(static_cast<float>(mouseMotionX), static_cast<float>(mouseMotionY));
				}
				// Click Derecho - Look around 
				else if (mouseButtons[SDL_BUTTON_RIGHT - 1] == KEY_REPEAT || mouseButtons[SDL_BUTTON_RIGHT - 1] == KEY_DOWN)
				{
					camera->HandleMouseInput(mouseXf, mouseYf);
				}
			}
		}
		break;

		// Drag and Drop - FBX& textures
		case SDL_EVENT_DROP_FILE:
			if (event.drop.data != nullptr)
			{
				droppedFilePath = event.drop.data;
				droppedFile = true;
				std::cout << "File dropped: " << droppedFilePath << std::endl;

				// Determine file type
				if (droppedFilePath.size() >= 4)
				{
					std::string extension = droppedFilePath.substr(droppedFilePath.size() - 4);
					for (char& c : extension) c = tolower(c);

					if (extension == ".fbx")
					{
						droppedFileType = DROPPED_FBX;
					}
					else if (extension == ".png" || extension == ".dds")
					{
						droppedFileType = DROPPED_TEXTURE;
					}
					else
					{
						droppedFileType = DROPPED_NONE;
					}
				}
			}
			break;

		case SDL_EVENT_MOUSE_WHEEL:
		{
			if (!imguiWantCaptureMouse)
			{
				Camera* camera = Application::GetInstance().renderer->GetCamera();
				camera->HandleScrollInput(static_cast<float>(event.wheel.y));
			}
		}
		break;
		}
	}

	// Tecla F - Focus en geometria (falta implementar logica seleccion)
	//if (keyboard[SDL_SCANCODE_F] == KEY_DOWN)
	//{
	//	Camera* camera = Application::GetInstance().renderer->GetCamera();
	//	glm::vec3 selectedObjectPosition(0.0f, 0.0f, 0.0f);
	//	float selectedObjectRadius = 1.0f; // Radio aproximado del objeto
	//	camera->FocusOnTarget(selectedObjectPosition, selectedObjectRadius);
	//}

	Camera* camera = Application::GetInstance().renderer->GetCamera();
	const float cameraBaseSpeed = 2.5f;
	float speedMultiplier = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] ? 2.0f : 1.0f;
	float cameraSpeed = cameraBaseSpeed * speedMultiplier * Application::GetInstance().time->GetDeltaTime();
	// Only active when you press right-click (WASD movement)
	if (!io.WantCaptureKeyboard && (mouseButtons[SDL_BUTTON_RIGHT - 1] == KEY_REPEAT || mouseButtons[SDL_BUTTON_RIGHT - 1] == KEY_DOWN))
	{
		glm::vec3 cameraFront = camera->GetFront();
		glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, camera->GetUp()));
		glm::vec3 cameraUp = camera->GetUp();

		// WASD - Movimiento horizontal
		if (keys[SDL_SCANCODE_W])
			camera->SetPosition(camera->GetPosition() + cameraSpeed * cameraFront);
		if (keys[SDL_SCANCODE_S])
			camera->SetPosition(camera->GetPosition() - cameraSpeed * cameraFront);
		if (keys[SDL_SCANCODE_A])
			camera->SetPosition(camera->GetPosition() - cameraRight * cameraSpeed);
		if (keys[SDL_SCANCODE_D])
			camera->SetPosition(camera->GetPosition() + cameraRight * cameraSpeed);
	}
	glm::vec3 cameraUp = camera->GetUp();
	// Up
	if (keys[SDL_SCANCODE_SPACE])
		camera->SetPosition(camera->GetPosition() + cameraUp * cameraSpeed);

	// Down
	if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL])
		camera->SetPosition(camera->GetPosition() - cameraUp * cameraSpeed);

	return true;
}

// Called before quitting
bool Input::CleanUp()
{
	SDL_QuitSubSystem(SDL_INIT_EVENTS);
	return true;
}

bool Input::GetWindowEvent(EventWindow ev)
{
	return windowEvents[ev];
}