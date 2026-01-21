#include "Input.h"
#include "Window.h"
#include "Application.h"
#include <iostream>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include <ImGuizmo.h>
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "ComponentParticleSystem.h"
#include "ComponentFirework.h"
#include <limits>
#include <random>

#define MAX_KEYS 300

Input::Input() : Module(), droppedFile(false), droppedFilePath(""), droppedFileType(DROPPED_NONE)
{
	keyboard = new KeyState[MAX_KEYS];
	memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
	memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
}

Input::~Input()
{
	delete[] keyboard;
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
			mouseButtons[event.button.button - 1] = KEY_UP;
			break;

		case SDL_EVENT_MOUSE_MOTION:
		{
			int scale = Application::GetInstance().window.get()->GetScale();
			mouseMotionX = static_cast<int>(event.motion.xrel / scale);
			mouseMotionY = static_cast<int>(event.motion.yrel / scale);
			mouseX = static_cast<int>(event.motion.x / scale);
			mouseY = static_cast<int>(event.motion.y / scale);

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

	// Firework creation when pressing "1"
	if (keyboard[SDL_SCANCODE_1] == KEY_DOWN)
	{
		CreateFirework();
	}

	return true;
}

bool Input::CleanUp()
{
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

	Octree* octree = scene->GetOctree();

	if (octree)
	{
		Ray ray(rayOrigin, rayDir);
		float distance;
		GameObject* picked = octree->RayPick(ray, distance);

		if (picked)
		{
			minDist = distance;
			scene->lastRayOrigin = rayOrigin;
			scene->lastRayDirection = rayDir;
			scene->lastRayLength = distance;

			return picked;
		}
		else
		{
			scene->lastRayOrigin = rayOrigin;
			scene->lastRayDirection = rayDir;
			scene->lastRayLength = 100.0f;

			float fallbackDist = std::numeric_limits<float>::max();
			GameObject* fallbackResult = FindClosestObjectToRay(root, rayOrigin, rayDir, fallbackDist);

			if (fallbackResult)
			{
				minDist = fallbackDist;
				return fallbackResult;
			}
		}

		return picked;
	}

	GameObject* result = FindClosestObjectToRay(root, rayOrigin, rayDir, minDist);
	return result;
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
					minDist = dist;
					closest = obj;
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

// Firework system implementation
void Input::CreateFirework()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);
	std::uniform_real_distribution<float> posXDist(-10.0f, 10.0f);
	std::uniform_real_distribution<float> posZDist(-10.0f, 10.0f);

	// Create the GameObject for the firework
	GameObject* fireworkGO = Application::GetInstance().scene->CreateGameObject("Firework Rocket");

	// Set random starting position (at ground level)
	Transform* transform = static_cast<Transform*>(fireworkGO->GetComponent(ComponentType::TRANSFORM));
	if (transform)
	{
		glm::vec3 startPos(posXDist(gen), 0.0f, posZDist(gen));
		transform->SetPosition(startPos);
	}

	// Generate two random bright colors
	glm::vec4 trailColor(colorDist(gen) * 0.5f + 0.5f, colorDist(gen) * 0.5f + 0.5f, colorDist(gen) * 0.3f, 1.0f);
	glm::vec4 explosionColor(colorDist(gen), colorDist(gen), colorDist(gen), 1.0f);
	float maxComponent = glm::max(glm::max(explosionColor.r, explosionColor.g), explosionColor.b);
	if (maxComponent < 0.6f)
	{
		explosionColor = explosionColor / maxComponent * 0.95f;
	}

	// Create particle system component
	ComponentParticleSystem* ps = static_cast<ComponentParticleSystem*>(
		fireworkGO->CreateComponent(ComponentType::PARTICLE_SYSTEM));

	if (ps)
	{
		// PHASE 1: TRAIL CONFIGURATION
		ParticleSystemConfig& config = ps->GetMainConfig();
		config.duration = 10.0f;
		config.looping = false;
		config.maxParticles = 800;
		config.startLifetime = 0.8f;
		config.startLifetimeVariance = 0.2f;
		config.startSpeed = 1.0f;
		config.startSpeedVariance = 0.5f;
		config.startSize = 0.3f;
		config.startSizeVariance = 0.1f;
		config.gravityModifier = 0.2f;
		config.simulationSpace = ParticleSystemConfig::SimulationSpace::WORLD;
		config.blendMode = ParticleSystemConfig::BlendMode::ADDITIVE;
		config.startColor = trailColor;

		// Emission: continuous trail handled here, explosion handled by preset later
		EmissionModule& emission = ps->GetEmissionModule();
		emission.enabled = true;
		emission.rateOverTime = 50.0f;
		emission.bursts.clear();

		// Shape: Narrow cone for trail
		ShapeModule& shape = ps->GetShapeModule();
		shape.enabled = true;
		shape.shape = EmitterShape::CONE;
		shape.coneAngle = 10.0f;
		shape.coneRadius = 0.1f;

		// Color over lifetime - fade trail
		ColorOverLifetimeModule& colorOverLife = ps->GetColorOverLifetime();
		colorOverLife.enabled = true;
		colorOverLife.startColor = trailColor;
		colorOverLife.endColor = glm::vec4(trailColor.r, trailColor.g, trailColor.b, 0.0f);

		// Size over lifetime - trail shrinks
		SizeOverLifetimeModule& sizeOverLife = ps->GetSizeOverLifetime();
		sizeOverLife.enabled = true;
		sizeOverLife.startSize = 1.0f;
		sizeOverLife.endSize = 0.0f;

		// Rotation
		RotationOverLifetimeModule& rotation = ps->GetRotationOverLifetime();
		rotation.enabled = true;
		rotation.angularVelocity = 90.0f;
		rotation.angularVelocityVariance = 45.0f;

		// Velocity - points down for trail effect
		VelocityOverLifetimeModule& velocity = ps->GetVelocityOverLifetime();
		velocity.enabled = true;
		velocity.velocity = glm::vec3(0.0f, -2.0f, 0.0f);
		velocity.isLocal = false;

		// Start the particle system
		ps->Play();

		LOG_CONSOLE("Firework rocket launched!");
	}

	// Add the firework control component
	ComponentFirework* fireworkComp = static_cast<ComponentFirework*>(
		fireworkGO->CreateComponent(ComponentType::FIREWORK));
	
	if (fireworkComp)
	{
		fireworkComp->SetLaunchParameters(12.0f, 2.0f, 10.0f);
		fireworkComp->explosionColor = explosionColor;
	}
}


