#include "Application.h"
#include "UIModule.h" 
#include "ModuleScene.h"
#include "Time.h"
#include <iostream>
#include <chrono>

Application::Application() : isRunning(true), playState(PlayState::EDITING)
{
    LOG_CONSOLE("Engine V3: Visual Systems & HUD Stage Active...");

    window = std::make_shared<Window>();
    input = std::make_shared<Input>();
    ui = std::make_shared<UIModule>();
    renderContext = std::make_shared<RenderContext>();
    renderer = std::make_shared<Renderer>();
    scene = std::make_shared<ModuleScene>();
    camera = std::make_shared<ModuleCamera>();
    editor = std::make_shared<ModuleEditor>();
    filesystem = std::make_shared<FileSystem>();
    time = std::make_shared<Time>();
    grid = std::make_shared<Grid>();
    resources = std::make_shared<ModuleResources>();

    AddModule(std::static_pointer_cast<Module>(window));
    AddModule(std::static_pointer_cast<Module>(input));
    AddModule(std::static_pointer_cast<Module>(renderContext));
    AddModule(std::static_pointer_cast<Module>(scene));
    AddModule(std::static_pointer_cast<Module>(camera));
    AddModule(std::static_pointer_cast<Module>(editor));
    AddModule(std::static_pointer_cast<Module>(ui)); 
    AddModule(std::static_pointer_cast<Module>(resources));
    AddModule(std::static_pointer_cast<Module>(filesystem));
    AddModule(std::static_pointer_cast<Module>(time));
    AddModule(std::static_pointer_cast<Module>(grid));
    AddModule(std::static_pointer_cast<Module>(renderer));

    selectionManager = new SelectionManager();
}

bool Application::DoUpdate()
{
    bool result = true;
    for (const auto& module : moduleList) {
        if (playState == PlayState::EDITING && module == scene) {
            continue;
        }

        result = module.get()->Update();
        if (!result) break;
    }
    return result;
}

void Application::Play()
{
    if (playState == PlayState::EDITING) {
        LOG_CONSOLE("V3: Creating scene snapshot for Play mode...");
        scene->SaveScene("../Library/TempScene/__v3_runtime_backup__.json");
    }

    playState = PlayState::PLAYING;
    time->Resume();
}

void Application::Stop()
{
    if (playState != PlayState::EDITING) {
        LOG_CONSOLE("V3: Restoring scene snapshot...");
        scene->LoadScene("../Library/TempScene/__v3_runtime_backup__.json");
    }

    playState = PlayState::EDITING;
    time->Reset();
    time->Pause();
}

bool Application::CleanUp()
{
    LOG_CONSOLE("V3: Initiating clean shutdown of all systems...");

    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->CleanUp();
        if (!result) break;
    }
    moduleList.clear();
    ui.reset(); 
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

    if (selectionManager) {
        delete selectionManager;
        selectionManager = nullptr;
    }

    ConsoleLog::GetInstance().Shutdown();
    return result;
}