#pragma once

#include "Module.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>
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

    bool ShouldShowVertexNormals() const { return showVertexNormals; }
    bool ShouldShowFaceNormals() const { return showFaceNormals; }

    ImVec2 sceneViewportPos;
    ImVec2 sceneViewportSize;

private:

    bool ShowMenuBar();

    // Configuration
    void DrawConfigurationWindow();
    void DrawFPSGraph();
    void DrawHardwareInfo();
    void DrawWindowSettings();
    void DrawCameraSettings();
    void DrawRendererSettings();

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

    void HandleDeleteKey();

    // Gizmo
    void DrawGizmo();
	void HandleGizmoInput();
private:

    // FPS
    std::vector<float> fpsHistory;
    const int maxFPSHistory = 100;
    float fpsTimer = 0.0f;

	// Windows
    bool showConsole = true;
    bool showConfiguration = true;
    bool showHierarchy = true;
    bool showInspector = true;
    bool showAbout = false;

    int lastWindowWidth = 0;
    int lastWindowHeight = 0;

	// Configuration
    bool fullscreen = false;
    float brightness = 1.0f;

    // Console
    bool showErrors = true;
    bool showWarnings = true;
    bool showInfo = true;
    bool showSuccess = true;
    bool showLoading = true;
    bool showLibraryInfo = true;
    bool autoScroll = true;
    bool scrollToBottom = false;
  
    
	// Hierarchy
    GameObject* renamingObject = nullptr;
    char renameBuffer[256] = "";

	// Inspector 
    bool showVertexNormals = false;
    bool showFaceNormals = false;

    // Camera Configuration
    float cameraMovementSpeed = 2.5f;
    float cameraMouseSensitivity = 0.2f;
    float cameraScrollSpeed = 0.5f;
    float cameraFOV = 45.0f;
    float cameraPanSensitivity = 0.003f;

    // ImGuizmo
	ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE; // Translate, Rotate, Scale
	ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;              // Local or World
};
