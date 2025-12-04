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
class GameWindow;
class GameObject;
struct Mesh;

enum class EditorWindowType
{
    NONE = 0,
    SCENE,
    GAME,
    CONFIGURATION,
    HIERARCHY,
    INSPECTOR,
    CONSOLE,
    ABOUT
};

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
    GameWindow* GetGameWindow() const { return gameWindow.get(); }
    ConfigurationWindow* GetConfigWindow() const { return configWindow.get(); }

    ImVec2 sceneViewportPos = ImVec2(0, 0);
    ImVec2 sceneViewportSize = ImVec2(1280, 720);

    ImVec2 gameViewportPos = ImVec2(0, 0);
    ImVec2 gameViewportSize = ImVec2(1280, 720);

    // Methods for accessing viewport and debug settings
    bool ShouldShowVertexNormals() const;
    bool ShouldShowFaceNormals() const;
    bool ShouldShowAABB() const;
    bool ShouldShowOctree() const;
    bool ShouldShowRaycast() const;

    // Window State
    EditorWindowType GetCurrentWindow() const { return currentWindow; }
    bool IsSceneWindowActive() const { return currentWindow == EditorWindowType::SCENE;  }
    bool IsMouseOverScene() const;

private:
    void ShowMenuBar();
    void ShowPlayToolbar();
    void DrawAboutWindow();
    void CreatePrimitiveGameObject(const std::string& name, Mesh mesh);
    void HandleDeleteKey();
    void UpdateCurrentWindow();
    const char* EditorWindowTypeToString(EditorWindowType type); // For debug // Delete before release

    // Editor windows (owned by ModuleEditor)
    std::unique_ptr<ConfigurationWindow> configWindow;
    std::unique_ptr<HierarchyWindow> hierarchyWindow;
    std::unique_ptr<InspectorWindow> inspectorWindow;
    std::unique_ptr<ConsoleWindow> consoleWindow;
    std::unique_ptr<SceneWindow> sceneWindow;
    std::unique_ptr<GameWindow> gameWindow;

    // About window state
    bool showAbout = false;

    // Window states
    EditorWindowType currentWindow = EditorWindowType::NONE;
    bool isMouseOverSceneViewport = false;
    EditorWindowType lastHoveredWindow = EditorWindowType::NONE;

};