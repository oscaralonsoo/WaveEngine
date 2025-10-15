#pragma once
#include "Module.h"
#include <string>

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

class Input : public Module
{

public:

	Input();

	// Destructor
	~Input();

	// Called before render is available
	bool Awake() override;

	// Called before the first frame
	bool Start() override;

	// Called each loop iteration
	bool PreUpdate() override;

	// Called before quitting
	bool CleanUp() override;

	// Check key states (includes mouse and joy buttons)
	KeyState GetKey(int id) const
	{
		return keyboard[id];
	}

	KeyState GetMouseButtonDown(int id) const
	{
		return mouseButtons[id - 1];
	}

	// Check if a certain window event happened
	bool GetWindowEvent(EventWindow ev);

	// Drag and Drop functions
	bool HasDroppedFile() const { return droppedFile; }
	const std::string& GetDroppedFilePath() const { return droppedFilePath; }
	void ClearDroppedFile() { droppedFile = false; droppedFilePath.clear(); }

	// Get mouse / axis position
	//Vector2D GetMousePosition();
	//Vector2D GetMouseMotion();

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
};