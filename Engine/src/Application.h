#pragma once

#include <memory>
#include <list>
#include "Window.h"
#include "Module.h"
#include "Input.h"
#include "RenderContext.h"
#include "Renderer.h"
#include "FileSystem.h"
#include "Time.h"
#include "ModuleScene.h"
#include "Grid.h"
#include "ModuleEditor.h"
#include "SelectionManager.h"
#include "ModuleCamera.h" 
#include "ModuleResources.h" 

class Module;

class Application
{
public:
    // Singleton instance
    static Application& GetInstance();

    // Called before render is available
    bool Awake();

    // Called before the first frame
    bool Start();

    // Called each loop iteration
    bool Update();

    // Called before quitting
    bool CleanUp();

    // Request application exit
    void RequestExit() { isRunning = false; }

    // Play mode control
    enum class PlayState
    {
        EDITING,
        PLAYING,
        PAUSED
    };

    void Play();
    void Pause();
    void Stop();
    void Step();
    PlayState GetPlayState() const { return playState; }

    // Modules
    std::shared_ptr<Window> window;
    std::shared_ptr<Input> input;
    std::shared_ptr<RenderContext> renderContext;
    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<FileSystem> filesystem;
    std::shared_ptr<Time> time;
    std::shared_ptr<ModuleScene> scene;
    std::shared_ptr<ModuleCamera> camera;
    std::shared_ptr<ModuleEditor> editor;
    std::shared_ptr<Grid> grid;
    std::shared_ptr<ModuleResources> resources;

    SelectionManager* selectionManager;

private:
    // Private constructor for singleton
    Application();
    ~Application() = default;

    // Delete copy constructor and assignment
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void AddModule(std::shared_ptr<Module> module);
    std::list<std::shared_ptr<Module>> moduleList;

    bool isRunning;
    PlayState playState;

    // Call modules before each loop iteration
    bool PreUpdate();

    // Call modules on each loop iteration
    bool DoUpdate();

    // Call modules after each loop iteration
    bool PostUpdate();

public:

    enum EngineState
    {
        CREATE = 1,
        AWAKE,
        START,
        LOOP,
        CLEAN,
        FAIL,
        EXIT
    };

};