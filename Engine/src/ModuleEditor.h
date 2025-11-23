#pragma once

#include "Module.h"
#include <imgui.h>
#include <memory>
#include <string>

// Forward declarations
class ConfigurationWindow;
class HierarchyWindow;
class InspectorWindow;
class ConsoleWindow;
class SceneWindow;
class GameObject;
struct Mesh;

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

    // Access to windows
    InspectorWindow* GetInspector() const { return inspectorWindow.get(); }
    SceneWindow* GetSceneWindow() const { return sceneWindow.get(); }
    ConfigurationWindow* GetConfigWindow() const { return configWindow.get(); }

    ImVec2 sceneViewportPos = ImVec2(0, 0);
    ImVec2 sceneViewportSize = ImVec2(1280, 720);

    // Methods for accessing viewport and debug settings
    bool ShouldShowVertexNormals() const;
    bool ShouldShowFaceNormals() const;
    bool ShouldShowAABB() const;
    bool ShouldShowOctree() const;
    bool ShouldShowRaycast() const;

private:
    void ShowMenuBar();
    void DrawAboutWindow();
    void CreatePrimitiveGameObject(const std::string& name, Mesh mesh);
    void HandleDeleteKey();

    // Editor windows (owned by ModuleEditor)
    std::unique_ptr<ConfigurationWindow> configWindow;
    std::unique_ptr<HierarchyWindow> hierarchyWindow;
    std::unique_ptr<InspectorWindow> inspectorWindow;
    std::unique_ptr<ConsoleWindow> consoleWindow;
    std::unique_ptr<SceneWindow> sceneWindow;

    // About window state
    bool showAbout = false;
};