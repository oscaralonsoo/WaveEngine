#pragma once
#include <memory>
#include <list>
#include "Module.h"
#include "Window.h"
#include "Input.h"
#include "Time.h"
#include "FileSystem.h"
#include "RenderContext.h"
#include "Renderer.h"
#include "ModuleScene.h"
#include "ModuleCamera.h" 
#include "Grid.h"
#include "ModuleEditor.h"
#include "UIModule.h" 
#include "ModuleResources.h" 
#include "SelectionManager.h"

class Module;

class Application
{
public:

    static Application& GetInstance();
    bool Awake();
    bool Start();
    bool Update();
    bool CleanUp();

    void RequestExit() { isRunning = false; }

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

public:

    std::shared_ptr<UIModule> ui; 
    std::shared_ptr<Window> window;
    std::shared_ptr<Input> input;
    
    std::shared_ptr<RenderContext> renderContext;
    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<ModuleScene> scene;
    std::shared_ptr<ModuleCamera> camera;
    std::shared_ptr<ModuleEditor> editor;
    
    std::shared_ptr<FileSystem> filesystem;
    std::shared_ptr<Time> time;
    std::shared_ptr<Grid> grid;
    std::shared_ptr<ModuleResources> resources;

    SelectionManager* selectionManager;

private:

    Application();
    ~Application() = default;

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void AddModule(std::shared_ptr<Module> module);
    std::list<std::shared_ptr<Module>> moduleList;

    bool isRunning;
    PlayState playState;

    bool PreUpdate();
    bool DoUpdate();
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