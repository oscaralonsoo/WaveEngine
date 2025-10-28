#pragma once
#include "Module.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

class ModuleEditor : public Module {

public:
	ModuleEditor();
	virtual ~ModuleEditor();

	bool Awake() override;
	bool Start() override;
	bool Update() override;
	bool PostUpdate() override;
	bool CleanUp() override;

private:
	bool ShowTest();

};
