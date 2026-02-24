#include "Input.h"
#include "Window.h"
#include "UI.h"
#include "Application.h"
#include "ModuleEditor.h"
#include <iostream>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include <ImGuizmo.h>
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include <limits>
#include <SDL3/SDL_scancode.h>

#define MAX_KEYS 300

Input* Input::instance = nullptr;

Input::Input() : Module(), droppedFile(false), droppedFilePath(""), droppedFileType(DROPPED_NONE)
{
	keyboard = new KeyState[MAX_KEYS];
	memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
	memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);

	instance = this;
}

Input::~Input()
{
	delete[] keyboard;
	instance = this;
}

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

bool Input::Start()
{
	LOG_DEBUG("Input::Start GAMEPAD INIT");

	if (SDL_InitSubSystem(SDL_INIT_GAMEPAD) < 0)
		LOG_DEBUG("SDL Gamepad init FAILED: %s", SDL_GetError());
	else
		LOG_DEBUG("SDL Gamepad init OK");

	int numJoysticks = 0;
	SDL_JoystickID* joysticks = SDL_GetJoysticks(&numJoysticks);
	LOG_DEBUG("Joysticks found: %d", numJoysticks);
	SDL_free(joysticks);

	return true;
}

bool IsMouseOverSceneWindow()
{
	ModuleEditor* editor = Application::GetInstance().editor.get();
	if (!editor) return false;

	return editor->IsMouseOverScene();
}
bool Input::PreUpdate()
{
	static SDL_Event event;
	const bool* keys = SDL_GetKeyboardState(NULL);

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
	//For script text editor
	SDL_StartTextInput(Application::GetInstance().window->GetWindow());

	while (SDL_PollEvent(&event))
	{
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
			mouseButtons[event.button.button - 1] = KEY_DOWN;

			// Send mouse button down to Noesis UI
			{
				float mx, my;
				SDL_GetMouseState(&mx, &my);
				int scale = Application::GetInstance().window.get()->GetScale();
				int uiMx = (int)(mx / scale);
				int uiMy = (int)(my / scale);

				ModuleEditor* editor = Application::GetInstance().editor.get();
				if (editor)
				{
					uiMx -= (int)editor->gameViewportPos.x;
					uiMy -= (int)editor->gameViewportPos.y;
				}

				Application::GetInstance().ui.get()->OnMouseButtonDown(
					uiMx, uiMy, event.button.button);
			}

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
			break;
		}
		case SDL_EVENT_MOUSE_BUTTON_UP:
		{
			mouseButtons[event.button.button - 1] = KEY_UP;

			// Send mouse button up to Noesis UI
			float mx, my;
			SDL_GetMouseState(&mx, &my);
			int scale = Application::GetInstance().window.get()->GetScale();
			int uiMx = (int)(mx / scale);
			int uiMy = (int)(my / scale);

			ModuleEditor* editor = Application::GetInstance().editor.get();
			if (editor)
			{
				uiMx -= (int)editor->gameViewportPos.x;
				uiMy -= (int)editor->gameViewportPos.y;
			}

			Application::GetInstance().ui.get()->OnMouseButtonUp(
				uiMx, uiMy, event.button.button);
			break;
		}

		case SDL_EVENT_MOUSE_MOTION:
		{
			int scale = Application::GetInstance().window.get()->GetScale();
			mouseMotionX = static_cast<int>(event.motion.xrel / scale);
			mouseMotionY = static_cast<int>(event.motion.yrel / scale);
			mouseX = static_cast<int>(event.motion.x / scale);
			mouseY = static_cast<int>(event.motion.y / scale);

			int uiMouseX = mouseX;
			int uiMouseY = mouseY;

			ModuleEditor* editor = Application::GetInstance().editor.get();
			if (editor)
			{
				uiMouseX -= (int)editor->gameViewportPos.x;
				uiMouseY -= (int)editor->gameViewportPos.y;
			}

			// Send mouse move to Noesis UI
			Application::GetInstance().ui.get()->SetMousePosition(uiMouseX, uiMouseY);

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
			break;
		}

		case SDL_EVENT_DROP_FILE:
			if (event.drop.data != nullptr)
			{
				droppedFilePath = event.drop.data;
				droppedFile = true;

				if (droppedFilePath.size() >= 4)
				{
					std::string extension = droppedFilePath.substr(droppedFilePath.size() - 4);
					for (char& c : extension) c = tolower(c);

					if (extension == ".fbx")
						droppedFileType = DROPPED_FBX;
					else if (extension == ".png" || extension == ".dds")
						droppedFileType = DROPPED_TEXTURE;
					else
						droppedFileType = DROPPED_NONE;
				}
			}
			break;

		case SDL_EVENT_MOUSE_WHEEL:
		{
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
			break;
		}
		 // ─── GAMEPAD ─── aquí, después del mouse wheel ───────────
    case SDL_EVENT_GAMEPAD_ADDED:
    {
        if (!gamepad)
        {
            gamepad = SDL_OpenGamepad(event.gdevice.which);
            LOG_DEBUG("Gamepad connected: %s", SDL_GetGamepadName(gamepad));
        }
        break;
    }
    case SDL_EVENT_GAMEPAD_REMOVED:
    {
        if (gamepad && SDL_GetGamepadID(gamepad) == event.gdevice.which)
        {
            SDL_CloseGamepad(gamepad);
            gamepad = nullptr;
            LOG_DEBUG("Gamepad disconnected");
            Application::GetInstance().ui->OnGamepadLeftStick(0.0f, 0.0f);
            Application::GetInstance().ui->OnGamepadRightStick(0.0f, 0.0f);
            Application::GetInstance().ui->OnGamepadTrigger(0.0f, 0.0f);
        }
        break;
    }
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    {
        Application::GetInstance().ui->OnGamepadButtonDown(event.gbutton.button);
        break;
    }
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
    {
        Application::GetInstance().ui->OnGamepadButtonUp(event.gbutton.button);
        break;
    }
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
    {
        float value = event.gaxis.value / 32767.0f;
        value = ApplyDeadzone(value);
        switch (event.gaxis.axis)
        {
        case SDL_GAMEPAD_AXIS_LEFTX:
            leftStickX = value;
            Application::GetInstance().ui->OnGamepadLeftStick(leftStickX, leftStickY);
            break;
        case SDL_GAMEPAD_AXIS_LEFTY:
            leftStickY = -value;
            Application::GetInstance().ui->OnGamepadLeftStick(leftStickX, leftStickY);
            break;
        case SDL_GAMEPAD_AXIS_RIGHTX:
            rightStickX = value;
            Application::GetInstance().ui->OnGamepadRightStick(rightStickX, rightStickY);
            break;
        case SDL_GAMEPAD_AXIS_RIGHTY:
            rightStickY = -value;
            Application::GetInstance().ui->OnGamepadRightStick(rightStickX, rightStickY);
            break;
        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
            triggerLeft = (value + 1.0f) * 0.5f;
            Application::GetInstance().ui->OnGamepadTrigger(triggerLeft, triggerRight);
            break;
        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
            triggerRight = (value + 1.0f) * 0.5f;
            Application::GetInstance().ui->OnGamepadTrigger(triggerLeft, triggerRight);
            break;
        }
        break;
    }
		}
	}

	// Camera movement with WASD
	ComponentCamera* camera = Application::GetInstance().camera->GetEditorCamera();
	if (!camera) return true;
	GameObject* cameraGO = camera->owner;
	Transform* cameraTransform = static_cast<Transform*>(cameraGO->GetComponent(ComponentType::TRANSFORM));
	if (!cameraTransform) return true;

	const float cameraBaseSpeed = camera->GetMovementSpeed();
	float speedMultiplier = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] ? 2.0f : 1.0f;
	float cameraSpeed = cameraBaseSpeed * speedMultiplier * Application::GetInstance().time->GetRealDeltaTime();

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
		//if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL])
		//	cameraPos -= cameraUp * cameraSpeed;

		cameraTransform->SetPosition(cameraPos);
	}

	if (keyboard[SDL_SCANCODE_F] == KEY_DOWN)
	{
		if (Application::GetInstance().selectionManager->HasSelection())
		{
			// Get all selected objects
			const std::vector<GameObject*>& selectedObjects =
				Application::GetInstance().selectionManager->GetSelectedObjects();

			if (!selectedObjects.empty())
			{
				// Calculate combined AABB that encompasses all selected objects
				glm::vec3 combinedMin(FLT_MAX);
				glm::vec3 combinedMax(-FLT_MAX);
				bool hasValidAABB = false;

				for (GameObject* obj : selectedObjects)
				{
					if (!obj) continue;

					ComponentMesh* mesh = static_cast<ComponentMesh*>(obj->GetComponent(ComponentType::MESH));
					Transform* transform = static_cast<Transform*>(obj->GetComponent(ComponentType::TRANSFORM));

					if (mesh && mesh->HasMesh() && transform)
					{
						glm::vec3 localMin = mesh->GetAABBMin();
						glm::vec3 localMax = mesh->GetAABBMax();
						glm::mat4 globalMatrix = transform->GetGlobalMatrix();
						// Transform the 8 corners of the local AABB to world space
						glm::vec3 corners[8] = {
							glm::vec3(globalMatrix * glm::vec4(localMin.x, localMin.y, localMin.z, 1.0f)),
							glm::vec3(globalMatrix * glm::vec4(localMax.x, localMin.y, localMin.z, 1.0f)),
							glm::vec3(globalMatrix * glm::vec4(localMin.x, localMax.y, localMin.z, 1.0f)),
							glm::vec3(globalMatrix * glm::vec4(localMax.x, localMax.y, localMin.z, 1.0f)),
							glm::vec3(globalMatrix * glm::vec4(localMin.x, localMin.y, localMax.z, 1.0f)),
							glm::vec3(globalMatrix * glm::vec4(localMax.x, localMin.y, localMax.z, 1.0f)),
							glm::vec3(globalMatrix * glm::vec4(localMin.x, localMax.y, localMax.z, 1.0f)),
							glm::vec3(globalMatrix * glm::vec4(localMax.x, localMax.y, localMax.z, 1.0f))
						};

						// Expand the AABB combined with all corners
						for (int i = 0; i < 8; ++i)
						{
							combinedMin = glm::min(combinedMin, corners[i]);
							combinedMax = glm::max(combinedMax, corners[i]);
						}

						hasValidAABB = true;
					}
					else if (transform)
					{
						// If there is no mesh, use only its position
						glm::mat4 globalMatrix = transform->GetGlobalMatrix();
						glm::vec3 position = glm::vec3(globalMatrix[3]);

						combinedMin = glm::min(combinedMin, position);
						combinedMax = glm::max(combinedMax, position);
						hasValidAABB = true;
					}
				}

				if (hasValidAABB)
				{
					// Calculate centre and radius of the combined AABB
					glm::vec3 worldCenter = (combinedMin + combinedMax) * 0.5f;
					glm::vec3 worldSize = combinedMax - combinedMin;

					// Use the maximum dimension of the combined AABB
					float maxDimension = glm::max(glm::max(worldSize.x, worldSize.y), worldSize.z);
					float radius = maxDimension * 0.5f;

					if (radius < 0.1f)
						radius = 1.0f;

					// Obtain the aspect ratio of the viewport
					ImVec2 sceneSize = Application::GetInstance().editor->sceneViewportSize;
					float viewportAspectRatio = sceneSize.x / sceneSize.y;

					camera->FocusOnTarget(worldCenter, radius, viewportAspectRatio);
					camera->SetOrbitTarget(worldCenter);
				}
			}
		}
	}

	return true;
}

bool Input::CleanUp()
{
    if (gamepad)
    {
        SDL_CloseGamepad(gamepad);
        gamepad = nullptr;
    }

    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
    return true;
}

bool Input::GetWindowEvent(EventWindow ev)
{
	return windowEvents[ev];
}

// Ray picking functions
GameObject* FindClosestObjectToRayOptimized(GameObject* root, const glm::vec3& rayOrigin,
	const glm::vec3& rayDir, float& minDist)
{
	ModuleScene* scene = Application::GetInstance().scene.get();
	if (!scene)
	{
		return nullptr;
	}

	GameObject* result = FindClosestObjectToRay(root, rayOrigin, rayDir, minDist);
	return result;
}

bool RayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
	const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
	float& t)
{
	const float EPSILON = 0.0000001f;
	glm::vec3 edge1, edge2, h, s, q;
	float a, f, u, v;
	edge1 = v1 - v0;
	edge2 = v2 - v0;
	h = glm::cross(rayDir, edge2);
	a = glm::dot(edge1, h);
	if (a > -EPSILON && a < EPSILON)
		return false;
	f = 1.0f / a;
	s = rayOrigin - v0;
	u = f * glm::dot(s, h);
	if (u < 0.0f || u > 1.0f)
		return false;
	q = glm::cross(s, edge1);
	v = f * glm::dot(rayDir, q);
	if (v < 0.0f || u + v > 1.0f)
		return false;
	t = f * glm::dot(edge2, q);
	return t > EPSILON;
}

GameObject* FindClosestObjectToRay(GameObject* obj, const glm::vec3& rayOrigin,
	const glm::vec3& rayDir, float& minDist)
{
	if (!obj || !obj->IsActive())
		return nullptr;

	GameObject* closest = nullptr;

	ComponentMesh* mesh = static_cast<ComponentMesh*>(obj->GetComponent(ComponentType::MESH));
	if (mesh && mesh->IsActive() && mesh->HasMesh())
	{
		Transform* t = static_cast<Transform*>(obj->GetComponent(ComponentType::TRANSFORM));
		if (t)
		{
			glm::vec3 localMin = mesh->GetAABBMin();
			glm::vec3 localMax = mesh->GetAABBMax();
			glm::mat4 globalMatrix = t->GetGlobalMatrix();

			glm::vec3 corners[8] = {
				glm::vec3(globalMatrix * glm::vec4(localMin.x, localMin.y, localMin.z, 1.0f)),
				glm::vec3(globalMatrix * glm::vec4(localMax.x, localMin.y, localMin.z, 1.0f)),
				glm::vec3(globalMatrix * glm::vec4(localMin.x, localMax.y, localMin.z, 1.0f)),
				glm::vec3(globalMatrix * glm::vec4(localMax.x, localMax.y, localMin.z, 1.0f)),
				glm::vec3(globalMatrix * glm::vec4(localMin.x, localMin.y, localMax.z, 1.0f)),
				glm::vec3(globalMatrix * glm::vec4(localMax.x, localMin.y, localMax.z, 1.0f)),
				glm::vec3(globalMatrix * glm::vec4(localMin.x, localMax.y, localMax.z, 1.0f)),
				glm::vec3(globalMatrix * glm::vec4(localMax.x, localMax.y, localMax.z, 1.0f))
			};

			glm::vec3 worldMin = corners[0];
			glm::vec3 worldMax = corners[0];
			for (int i = 1; i < 8; ++i)
			{
				worldMin = glm::min(worldMin, corners[i]);
				worldMax = glm::max(worldMax, corners[i]);
			}

			float dist;
			if (RayIntersectsAABB(rayOrigin, rayDir, worldMin, worldMax, dist))
			{
				if (dist < minDist)
				{
				// Check against actual mesh triangles for precision
				const Mesh& m = mesh->GetMesh();
				bool hitMesh = false;
				float localMinDist = std::numeric_limits<float>::max();

				if (!m.indices.empty())
				{
					for (size_t i = 0; i < m.indices.size(); i += 3)
					{
						glm::vec3 v0 = glm::vec3(globalMatrix * glm::vec4(m.vertices[m.indices[i]].position, 1.0f));
						glm::vec3 v1 = glm::vec3(globalMatrix * glm::vec4(m.vertices[m.indices[i + 1]].position, 1.0f));
						glm::vec3 v2 = glm::vec3(globalMatrix * glm::vec4(m.vertices[m.indices[i + 2]].position, 1.0f));

						float t_tri;
						if (RayIntersectsTriangle(rayOrigin, rayDir, v0, v1, v2, t_tri))
						{
							if (t_tri < localMinDist)
							{
								localMinDist = t_tri;
								hitMesh = true;
							}
						}
					}
				}
				else
				{
					for (size_t i = 0; i < m.vertices.size(); i += 3)
					{
						glm::vec3 v0 = glm::vec3(globalMatrix * glm::vec4(m.vertices[i].position, 1.0f));
						glm::vec3 v1 = glm::vec3(globalMatrix * glm::vec4(m.vertices[i + 1].position, 1.0f));
						glm::vec3 v2 = glm::vec3(globalMatrix * glm::vec4(m.vertices[i + 2].position, 1.0f));

						float t_tri;
						if (RayIntersectsTriangle(rayOrigin, rayDir, v0, v1, v2, t_tri))
						{
							if (t_tri < localMinDist)
							{
								localMinDist = t_tri;
								hitMesh = true;
							}
						}
					}
				}

				if (hitMesh && localMinDist < minDist)
				{
					minDist = localMinDist;
					closest = obj;
				}
				}
			}
		}
	}

	for (GameObject* child : obj->GetChildren())
	{
		GameObject* childResult = FindClosestObjectToRay(child, rayOrigin, rayDir, minDist);
		if (childResult) closest = childResult;
	}

	return closest;
}

bool Input::IsKeyPressed(int scancode)
{
	if (!instance) return false;
	return instance->GetKey(scancode) == KEY_REPEAT || instance->GetKey(scancode) == KEY_DOWN;
}

bool Input::IsKeyDown(int scancode)
{
	if (!instance) return false;
	return instance->GetKey(scancode) == KEY_DOWN;
}

void Input::GetMousePosition(int& x, int& y)
{
	if (!instance) {
		x = 0;
		y = 0;
		return;
	}
	x = instance->mouseX;
	y = instance->mouseY;
}

float Input::ApplyDeadzone(float value, float deadzone)
{
    if (fabs(value) < deadzone) return 0.0f;
    float sign = value > 0.0f ? 1.0f : -1.0f;
    return sign * (fabs(value) - deadzone) / (1.0f - deadzone);
}