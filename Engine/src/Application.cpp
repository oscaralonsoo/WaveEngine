#include "Application.h"
#include <iostream>
#include <chrono>

Application::Application() : isRunning(true), playState(PlayState::EDITING)
{
    LOG_DEBUG("=== Creating Application Instance ===");
    LOG_CONSOLE("Starting engine...");

    window = std::make_shared<Window>();
    input = std::make_shared<Input>();
    renderContext = std::make_shared<RenderContext>();
    renderer = std::make_shared<Renderer>();
    scene = std::make_shared<ModuleScene>();
    camera = std::make_shared<ModuleCamera>();
    editor = std::make_shared<ModuleEditor>();
    filesystem = std::make_shared<FileSystem>();
    time = std::make_shared<Time>();
    grid = std::make_shared<Grid>();
    resources = std::make_shared<ModuleResources>();
    //script = std::make_shared<ModuleScripting>();

    AddModule(std::static_pointer_cast<Module>(window));
    AddModule(std::static_pointer_cast<Module>(input));
    AddModule(std::static_pointer_cast<Module>(renderContext));
    AddModule(std::static_pointer_cast<Module>(scene));
    AddModule(std::static_pointer_cast<Module>(camera));
    AddModule(std::static_pointer_cast<Module>(editor));
    AddModule(std::static_pointer_cast<Module>(resources));
    AddModule(std::static_pointer_cast<Module>(filesystem));
    AddModule(std::static_pointer_cast<Module>(time));
    AddModule(std::static_pointer_cast<Module>(grid));
    AddModule(std::static_pointer_cast<Module>(renderer));
   //AddModule(std::static_pointer_cast<Module>(script));

    selectionManager = new SelectionManager();
}

Application& Application::GetInstance()
{
    static Application instance;
    return instance;
}

void Application::AddModule(std::shared_ptr<Module> module)
{
    moduleList.push_back(module);
}

bool Application::Awake()
{
    return true;
}

bool Application::Start()
{
    LOG_CONSOLE("Starting engine modules...");
    LOG_CONSOLE("========================================");

    auto totalStart = std::chrono::high_resolution_clock::now();

    bool result = true;
    for (const auto& module : moduleList) {
        auto moduleStart = std::chrono::high_resolution_clock::now();

        LOG_CONSOLE("Initializing: %s...", module->name.c_str());

        result = module.get()->Start();

        auto moduleEnd = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(moduleEnd - moduleStart).count();

        LOG_CONSOLE("  -> %s: %lld ms", module->name.c_str(), duration);

        if (!result) {
            LOG_DEBUG("ERROR: Module failed to start: %s", module->name.c_str());
            LOG_CONSOLE("ERROR: Failed to initialize module: %s", module->name.c_str());
            break;
        }
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();

    LOG_CONSOLE("========================================");
    LOG_CONSOLE("TOTAL INIT TIME: %lld ms", totalDuration);
    LOG_CONSOLE("========================================");

    if (result)
    {
        LOG_CONSOLE("Engine ready - All systems initialized");
    }

    return true;
}

bool Application::Update()
{
    // Check if exit was requested from menu first
    if (!isRunning) {
        LOG_DEBUG("Exit requested by user");
        LOG_CONSOLE("Shutting down...");
        return false;
    }

    bool ret = true;

    if (input->GetWindowEvent(WE_QUIT) == true) {
        LOG_DEBUG("Window close event detected");
        LOG_CONSOLE("Shutting down...");
        ret = false;
    }

    if (ret == true)
        ret = PreUpdate();

    if (ret == true)
        ret = DoUpdate();

    if (ret == true)
        ret = PostUpdate();

    return ret;
}

bool Application::PreUpdate()
{
    //Iterates the module list and calls PreUpdate on each module
    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->PreUpdate();
        if (!result) {
            break;
        }
    }

    return result;
}

// Call modules on each loop iteration
bool Application::DoUpdate()
{
    //Iterates the module list and calls Update on each module
    bool result = true;
    for (const auto& module : moduleList) {
        // Skip scene updates when in editing mode
        if (playState == PlayState::EDITING && module == scene) {
            continue;
        }

        result = module.get()->Update();
        if (!result) {
            break;
        }
    }

    return result;
}

// Call modules after each loop iteration
bool Application::PostUpdate()
{
    //Iterates the module list and calls PostUpdate on each module
    bool result = true;

    for (const auto& module : moduleList) {
        if (module == window) {
            continue;
        }

        result = module.get()->PostUpdate();
        if (!result) {
            break;
        }
    }

    if (result) {
        result = window->PostUpdate();
    }

    return result;
}

void Application::Play()
{
    // Save
    if (playState == PlayState::EDITING) {
        LOG_CONSOLE("Saving initial scene state...");
        scene->SaveScene("../Library/TempScene/__temp_scene_state__.json");
    }

    playState = PlayState::PLAYING;
    time->Resume();
}

void Application::Pause()
{
    playState = PlayState::PAUSED;
    time->Pause();
}

void Application::Stop()
{
    // Restore
    if (playState != PlayState::EDITING) {
        LOG_CONSOLE("Restoring initial scene state...");
        scene->LoadScene("../Library/TempScene/__temp_scene_state__.json");
    }

    playState = PlayState::EDITING;
    time->Reset();
    time->Pause();
}

void Application::Step()
{
    if (playState != PlayState::EDITING)
    {
        time->StepFrame();
    }
}

bool Application::CleanUp()
{
    LOG_DEBUG("=== Cleaning Up Application ===");
    LOG_CONSOLE("Cleaning up modules...");

    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->CleanUp();
        if (!result) {
            break;
        }
    }
    moduleList.clear();

    editor.reset();
    camera.reset();
    scene.reset();
    renderer.reset();
    renderContext.reset();
    grid.reset();
    time.reset();
    filesystem.reset();
    input.reset();
    window.reset();
    resources.reset();

    delete selectionManager;
    selectionManager = nullptr;

    ConsoleLog::GetInstance().Shutdown();

    LOG_DEBUG("=== Application Cleanup Complete ===");
    LOG_CONSOLE("Shutdown complete");
    return result;
}