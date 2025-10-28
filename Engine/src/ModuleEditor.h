#pragma once
#include "Module.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <vector>

class ModuleEditor : public Module
{
public:
    ModuleEditor();
    ~ModuleEditor();

    bool Start() override;
    bool PreUpdate() override;
    bool Update() override;
    bool PostUpdate() override;
    bool CleanUp() override;

private:

    // Configuraci��n
    void DrawConfigurationWindow();
    void DrawFPSGraph();
    void DrawHardwareInfo();
    void DrawWindowSettings();

    // Jerarq��a
    void DrawHierarchyWindow();

    // Inspector
    void DrawInspectorWindow();

    // Consola
    void DrawConsoleWindow();

private:

    bool fullscreen = false;
    float brightness = 1.0f;
    
    // FPS
    std::vector<float> fpsHistory;
    const int maxFPSHistory = 100;
    float fpsTimer = 0.0f;

	bool ShowMenuBar();
	bool ShowTest();

    bool showConsole = true;
    bool showConfiguration = true;
    bool showHierarchy = true;
    bool showInspector = true;
    bool showAbout = false;

};