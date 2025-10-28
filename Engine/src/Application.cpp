#include "Application.h"
#include <iostream>

Application::Application() : isRunning(true)
{
    std::cout << "Application Constructor" << std::endl;
    window = std::make_shared<Window>();
    input = std::make_shared<Input>();
    renderContext = std::make_shared<RenderContext>();
    renderer = std::make_shared<Renderer>();
    scene = std::make_shared<ModuleScene>();
    editor = std::make_shared<ModuleEditor>();
    filesystem = std::make_shared<FileSystem>();
    time = std::make_shared<Time>();
    grid = std::make_shared<Grid>();

    AddModule(std::static_pointer_cast<Module>(window));
    AddModule(std::static_pointer_cast<Module>(input));
    AddModule(std::static_pointer_cast<Module>(renderContext));
    AddModule(std::static_pointer_cast<Module>(renderer));
    AddModule(std::static_pointer_cast<Module>(scene));
    AddModule(std::static_pointer_cast<Module>(editor));
    AddModule(std::static_pointer_cast<Module>(filesystem));
    AddModule(std::static_pointer_cast<Module>(time));
    AddModule(std::static_pointer_cast<Module>(grid));

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
    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->Start();
        if (!result) {
            break;
        }
    }

    return true;
}

bool Application::Update()
{
    // Check if exit was requested from menu first
    if (!isRunning)
        return false;

    bool ret = true;

    if (input->GetWindowEvent(WE_QUIT) == true)
        ret = false;

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

bool Application::CleanUp()
{
    std::cout << "Application CleanUp" << std::endl;

    bool result = true;
    for (const auto& module : moduleList) {
        result = module.get()->CleanUp();
        if (!result) {
            break;
        }
    }

    return result;
}