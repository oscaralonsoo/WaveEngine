#pragma once
#include "Module.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <vector>
#include <string>
#include "ComponentMesh.h"

class GameObject;

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

    bool ShowMenuBar();

    // Configuration
    void DrawConfigurationWindow();
    void DrawFPSGraph();
    void DrawHardwareInfo();
    void DrawWindowSettings();

    // Hierarchy
    void DrawHierarchyWindow();
    void DrawGameObjectNode(GameObject* gameObject);
    void SelectGameObjectAndChildren(GameObject* gameObject);


    // Inspector
    void DrawInspectorWindow();

    // Console
    void DrawConsoleWindow();

    // About
    void DrawAboutWindow();

    // Primitives
    void CreatePrimitiveGameObject(const std::string& name, Mesh mesh);

private:

    // FPS
    std::vector<float> fpsHistory;
    const int maxFPSHistory = 100;
    float fpsTimer = 0.0f;

    bool showConsole = true;
    bool showConfiguration = true;
    bool showHierarchy = true;
    bool showInspector = true;
    bool showAbout = false;

    bool fullscreen = false;
    float brightness = 1.0f;

    bool showErrors = true;
    bool showWarnings = true;
    bool showInfo = true;
    bool showSuccess = true;
    bool showLoading = true;
    bool showLibraryInfo = true;
    bool autoScroll = true;
    bool scrollToBottom = false;
    int lastWindowWidth = 0;
    int lastWindowHeight = 0;

    GameObject* renamingObject = nullptr;
    char renameBuffer[256] = "";
};
