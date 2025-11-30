#include "ModuleEditor.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <SDL3/SDL.h>
#include <IL/il.h>

#include "Application.h"
#include "Log.h"
#include "GameObject.h"
#include "SelectionManager.h"
#include "ModuleScene.h"
#include "Primitives.h"
#include "ComponentMesh.h"
#include "Transform.h"           
#include "ComponentCamera.h"   
#include "ComponentMaterial.h"

#include "ConfigurationWindow.h"
#include "HierarchyWindow.h"
#include "InspectorWindow.h"
#include "ConsoleWindow.h"
#include "SceneWindow.h"

ModuleEditor::ModuleEditor() : Module()
{
    name = "ModuleEditor";
}

ModuleEditor::~ModuleEditor()
{
}

bool ModuleEditor::Start()
{
    LOG_DEBUG("Initializing Editor");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL3_InitForOpenGL(
        Application::GetInstance().window->GetWindow(),
        Application::GetInstance().renderContext->GetContext()
    );
    ImGui_ImplOpenGL3_Init();

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.7f, 0.5f, 0.6f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.7f, 0.9f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.6f, 0.4f, 0.5f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.7f, 0.5f, 0.6f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(1.0f, 0.7f, 0.9f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(1.0f, 0.8f, 0.95f, 1.0f);

    // Create all windows
    configWindow = std::make_unique<ConfigurationWindow>();
    hierarchyWindow = std::make_unique<HierarchyWindow>();
    inspectorWindow = std::make_unique<InspectorWindow>();
    consoleWindow = std::make_unique<ConsoleWindow>();
    sceneWindow = std::make_unique<SceneWindow>(inspectorWindow.get());

    LOG_CONSOLE("Editor initialized");

    return true;
}

bool ModuleEditor::PreUpdate()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    return true;
}

bool ModuleEditor::Update()
{
    // Create fullscreen dockspace window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ShowMenuBar();

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
    
    sceneWindow->Draw(); 
    configWindow->Draw();
    consoleWindow->Draw();
    hierarchyWindow->Draw();
    inspectorWindow->Draw();

    if (showAbout) {
        DrawAboutWindow();
    }


    HandleDeleteKey();

    if (sceneWindow)
    {
        sceneViewportPos = sceneWindow->GetViewportPos();
        sceneViewportSize = sceneWindow->GetViewportSize();
    }

    UpdateCurrentWindow();

    // For debug // Delete before release
    //LOG_CONSOLE("%d", isMouseOverSceneViewport);
    //LOG_CONSOLE("%s",EditorWindowTypeToString(lastHoveredWindow));

    return true;
}

bool ModuleEditor::PostUpdate()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return true;
}

bool ModuleEditor::CleanUp()
{
    LOG_DEBUG("Cleaning up Editor");

    // Windows will be automatically destroyed by unique_ptr

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    return true;
}

void ModuleEditor::ShowMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                Application::GetInstance().RequestExit();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            bool configOpen = configWindow->IsOpen();
            if (ImGui::MenuItem("Configuration", NULL, &configOpen))
            {
                configWindow->SetOpen(configOpen);
            }

            bool consoleOpen = consoleWindow->IsOpen();
            if (ImGui::MenuItem("Console", NULL, &consoleOpen))
            {
                consoleWindow->SetOpen(consoleOpen);
            }

            bool hierarchyOpen = hierarchyWindow->IsOpen();
            if (ImGui::MenuItem("Hierarchy", NULL, &hierarchyOpen))
            {
                hierarchyWindow->SetOpen(hierarchyOpen);
            }

            bool inspectorOpen = inspectorWindow->IsOpen();
            if (ImGui::MenuItem("Inspector", NULL, &inspectorOpen))
            {
                inspectorWindow->SetOpen(inspectorOpen);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Camera"))
        {
            if (ImGui::MenuItem("Create Camera"))
            {
                Application& app = Application::GetInstance();
                GameObject* cameraGO = app.scene->CreateGameObject("Camera");

                Transform* transform = static_cast<Transform*>(
                    cameraGO->GetComponent(ComponentType::TRANSFORM)
                    );
                if (transform)
                {
                    transform->SetPosition(glm::vec3(0.0f, 1.5f, 10.0f));
                }

                ComponentCamera* sceneCamera = static_cast<ComponentCamera*>(
                    cameraGO->CreateComponent(ComponentType::CAMERA)
                    );

                if (sceneCamera)
                {
                    app.camera->SetSceneCamera(sceneCamera);
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObject"))
        {
            if (ImGui::BeginMenu("Create Primitive"))
            {
                if (ImGui::MenuItem("Cube"))
                {
                    CreatePrimitiveGameObject("Cube", Primitives::CreateCube());
                }
                if (ImGui::MenuItem("Pyramid"))
                {
                    CreatePrimitiveGameObject("Pyramid", Primitives::CreatePyramid());
                }
                if (ImGui::MenuItem("Plane"))
                {
                    CreatePrimitiveGameObject("Plane", Primitives::CreatePlane());
                }
                if (ImGui::MenuItem("Sphere"))
                {
                    CreatePrimitiveGameObject("Sphere", Primitives::CreateSphere());
                }
                if (ImGui::MenuItem("Cylinder"))
                {
                    CreatePrimitiveGameObject("Cylinder", Primitives::CreateCylinder());
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Documentation"))
            {
                SDL_OpenURL("https://github.com/Audra0000/Engine/blob/main/README.md");
            }
            if (ImGui::MenuItem("Report a Bug"))
            {
                SDL_OpenURL("https://github.com/Audra0000/Engine/issues");
            }
            if (ImGui::MenuItem("Download Latest"))
            {
                SDL_OpenURL("https://github.com/Audra0000/Engine/releases");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("About"))
            {
                showAbout = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void ModuleEditor::DrawAboutWindow()
{
    ImGui::Begin("About", &showAbout);

    ImGui::Text("Engine Name: Wave Engine");
    ImGui::Text("Version: 0.1.0");

    ImGui::Separator();

    ImGui::Text("Team Members:");
    ImGui::BulletText("Haosheng Li");
    ImGui::BulletText("Ana Alcaraz");

    ImGui::Separator();

    ImGui::Text("Libraries Used:");

    int sdlVersion = SDL_GetVersion();
    int major = SDL_VERSIONNUM_MAJOR(sdlVersion);
    int minor = SDL_VERSIONNUM_MINOR(sdlVersion);
    int patch = SDL_VERSIONNUM_MICRO(sdlVersion);
    ImGui::BulletText("SDL3: %d.%d.%d", major, minor, patch);

    ImGui::BulletText("OpenGL: %s", glGetString(GL_VERSION));
    ImGui::BulletText("ImGui: %s", IMGUI_VERSION);

    ILint devilVersion = ilGetInteger(IL_VERSION_NUM);
    int devilMajor = devilVersion / 100;
    int devilMinor = (devilVersion / 10) % 10;
    int devilPatch = devilVersion % 10;
    ImGui::BulletText("DevIL: %d.%d.%d", devilMajor, devilMinor, devilPatch);

    ImGui::BulletText("Glad");
    ImGui::BulletText("Glm");
    ImGui::BulletText("Assimp");

    ImGui::Separator();

    ImGui::TextWrapped("MIT License");
    ImGui::Spacing();
    ImGui::TextWrapped("Copyright (c) 2025 Engine");
    ImGui::Spacing();
    ImGui::TextWrapped(
        "Permission is hereby granted, free of charge, to any person obtaining a copy "
        "of this software and associated documentation files (the \"Software\"), to deal "
        "in the Software without restriction, including without limitation the rights "
        "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
        "copies of the Software, and to permit persons to whom the Software is "
        "furnished to do so, subject to the following conditions:"
    );
    ImGui::Spacing();
    ImGui::TextWrapped(
        "The above copyright notice and this permission notice shall be included in all "
        "copies or substantial portions of the Software."
    );
    ImGui::Spacing();
    ImGui::TextWrapped(
        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
        "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
        "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
        "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
        "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
        "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
        "SOFTWARE."
    );

    ImGui::End();
}

void ModuleEditor::CreatePrimitiveGameObject(const std::string& name, Mesh mesh)
{
    GameObject* Object = new GameObject(name);
    ComponentMesh* meshComp = static_cast<ComponentMesh*>(
        Object->CreateComponent(ComponentType::MESH)
        );

    Mesh selectedMesh;

    if (name == "Cube")
        selectedMesh = Primitives::CreateCube();
    else if (name == "Pyramid")
        selectedMesh = Primitives::CreatePyramid();
    else if (name == "Plane")
        selectedMesh = Primitives::CreatePlane();
    else if (name == "Sphere")
        selectedMesh = Primitives::CreateSphere();
    else if (name == "Cylinder")
        selectedMesh = Primitives::CreateCylinder();
    else
        selectedMesh = mesh;

    meshComp->SetMesh(selectedMesh);

    ComponentMaterial* materialComp = static_cast<ComponentMaterial*>(
        Object->CreateComponent(ComponentType::MATERIAL)
        );

    GameObject* root = Application::GetInstance().scene->GetRoot();
    root->AddChild(Object);

    Application::GetInstance().scene->RebuildOctree();

    LOG_CONSOLE("%s created", name.c_str());
    LOG_DEBUG("Primitive created: %s", name.c_str());
}

void ModuleEditor::HandleDeleteKey()
{
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
    {
        if (ImGui::GetIO().WantTextInput) return;

        std::vector<GameObject*> selectedObjects =
            Application::GetInstance().selectionManager->GetSelectedObjects();

        if (!selectedObjects.empty())
        {
            int deletedCount = 0;

            for (GameObject* obj : selectedObjects)
            {
                if (obj != nullptr && obj != Application::GetInstance().scene->GetRoot())
                {
                    obj->MarkForDeletion();
                    deletedCount++;
                }
            }

            Application::GetInstance().selectionManager->ClearSelection();

            if (deletedCount > 0)
            {
                LOG_CONSOLE("GameObject deleted: %d", deletedCount);
            }
        }
    }
}

bool ModuleEditor::ShouldShowVertexNormals() const
{
    if (inspectorWindow)
        return inspectorWindow->ShouldShowVertexNormals();
    return false;
}

bool ModuleEditor::ShouldShowFaceNormals() const
{
    if (inspectorWindow)
        return inspectorWindow->ShouldShowFaceNormals();
    return false;
}

bool ModuleEditor::ShouldShowAABB() const
{
    if (configWindow)
        return configWindow->ShouldShowAABB();
    return false;
}

bool ModuleEditor::ShouldShowOctree() const
{
    if (configWindow)
        return configWindow->ShouldShowOctree();
    return false;
}

bool ModuleEditor::ShouldShowRaycast() const
{
    if (configWindow)
        return configWindow->ShouldShowRaycast();
    return false;
}

void ModuleEditor::UpdateCurrentWindow()
{
    ImVec2 mousePos = ImGui::GetMousePos();

    isMouseOverSceneViewport = false;

    currentWindow = lastHoveredWindow;
    lastHoveredWindow = EditorWindowType::NONE;

    if (sceneWindow && sceneWindow->IsHovered()) {
        lastHoveredWindow = EditorWindowType::SCENE;
    }

    if (configWindow && configWindow->IsHovered()) {
        lastHoveredWindow = EditorWindowType::CONFIGURATION;
    }

    if (inspectorWindow && inspectorWindow->IsHovered()) {
        lastHoveredWindow = EditorWindowType::INSPECTOR;
    }

    if (consoleWindow && consoleWindow->IsHovered()) {
        lastHoveredWindow = EditorWindowType::CONSOLE;
    }

    if (hierarchyWindow && hierarchyWindow->IsHovered()) {
        lastHoveredWindow = EditorWindowType::HIERARCHY;
    }

    if (lastHoveredWindow != EditorWindowType::NONE) {
        currentWindow = lastHoveredWindow;
    }

    if (currentWindow == EditorWindowType::SCENE || currentWindow == EditorWindowType::NONE)
    {
        if (mousePos.x >= sceneViewportPos.x &&
            mousePos.x <= sceneViewportPos.x + sceneViewportSize.x &&
            mousePos.y >= sceneViewportPos.y &&
            mousePos.y <= sceneViewportPos.y + sceneViewportSize.y)
        {
            currentWindow = EditorWindowType::SCENE;
            isMouseOverSceneViewport = true;
        }
    }
}

bool ModuleEditor::IsMouseOverScene() const
{
    return isMouseOverSceneViewport;
}

const char* ModuleEditor::EditorWindowTypeToString(EditorWindowType type)
{
    switch (type)
    {
    case EditorWindowType::NONE:           return "NONE";
    case EditorWindowType::SCENE:          return "SCENE";
    case EditorWindowType::CONFIGURATION:  return "CONFIGURATION";
    case EditorWindowType::HIERARCHY:      return "HIERARCHY";
    case EditorWindowType::INSPECTOR:      return "INSPECTOR";
    case EditorWindowType::CONSOLE:        return "CONSOLE";
    case EditorWindowType::ABOUT:          return "ABOUT";
    default:                               return "UNKNOWN";
    }
}
