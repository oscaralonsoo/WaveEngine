#pragma once
#include "Module.h"
#include <string>
#include <glm/glm.hpp>
#include "Globals.h"

class GameObject;

#define NUM_MOUSE_BUTTONS 5

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

enum DroppedFileType
{
	DROPPED_NONE = 0,
	DROPPED_FBX,
	DROPPED_TEXTURE
};

GameObject* FindClosestObjectToRay(GameObject* obj, const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& minDist);
GameObject* FindClosestObjectToRayOptimized(GameObject* obj, const glm::vec3& rayOrigin, const glm::vec3& rayDir, float& minDist);

class Input : public Module
{
public:

	Input();
	~Input();

	bool Awake() override;
	bool Start() override;
	bool PreUpdate() override;
	bool CleanUp() override;

	KeyState GetKey(int id) const
	{
		return keyboard[id];
	}

	KeyState GetMouseButtonDown(int id) const
	{
		return mouseButtons[id - 1];
	}

	bool GetWindowEvent(EventWindow ev);

	// Drag and Drop functions
	bool HasDroppedFile() const { return droppedFile; }
	const std::string& GetDroppedFilePath() const { return droppedFilePath; }
	DroppedFileType GetDroppedFileType() const { return droppedFileType; }
	void ClearDroppedFile() { droppedFile = false; droppedFilePath.clear(); droppedFileType = DROPPED_NONE; }

	int GetMouseX() const { return mouseX; }
	int GetMouseY() const { return mouseY; }

	static bool IsKeyPressed(int scancode);
	static bool IsKeyDown(int scancode);
	static void GetMousePosition(int& x, int& y);

private:
	bool windowEvents[WE_COUNT];
	KeyState* keyboard;
	KeyState mouseButtons[NUM_MOUSE_BUTTONS];
	int	mouseMotionX;
	int mouseMotionY;
	int mouseX;
	int mouseY;

	// Drag and Drop
	bool droppedFile;
	std::string droppedFilePath;
	DroppedFileType droppedFileType;

	static Input* instance;
	friend class Application;
};