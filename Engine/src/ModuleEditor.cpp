#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <IL/il.h>
#include <windows.h>
#include <psapi.h>
#include <gl/GL.h>
#include <SDL3/SDL_timer.h>

#include "ModuleEditor.h"
#include "Application.h"
#include "Log.h"
#include "GameObject.h"
#include "SelectionManager.h"
#include "ModuleScene.h"
#include "Primitives.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "Transform.h"


ModuleEditor::ModuleEditor() : Module()
{
    name = "ModuleEditor";
    fpsHistory.reserve(maxFPSHistory);
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
    //io.IniFilename = NULL;  // for testing
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL3_InitForOpenGL(Application::GetInstance().window->GetWindow(), Application::GetInstance().renderContext->GetContext());
    ImGui_ImplOpenGL3_Init();

    ImGui::StyleColorsDark();

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

    // Menu Bar
    if (ImGui::BeginMenuBar()) {
        ShowMenuBar();
        ImGui::EndMenuBar();
    }

    // Create DockSpace
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();

    // Scene Window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Scene");

    sceneViewportPos = ImGui::GetCursorScreenPos();
    sceneViewportSize = ImGui::GetContentRegionAvail();

    GLuint sceneTexture = Application::GetInstance().renderer->GetSceneTexture();
    if (sceneTexture != 0 && sceneViewportSize.x > 0 && sceneViewportSize.y > 0)
    {
        // Display the scene texture
        ImTextureID texID = (ImTextureID)(uintptr_t)sceneTexture;
        ImGui::Image(texID, sceneViewportSize, ImVec2(0, 1), ImVec2(1, 0));
    }
    else
    {
        // Fallback: reserve space if texture is not ready
        ImGui::InvisibleButton("SceneView", sceneViewportSize);
    }

    ImGui::End();
    ImGui::PopStyleVar();

    // Configuration Window
    if (showConfiguration) {
        DrawConfigurationWindow();
    }

    // Console Window
    if (showConsole) {
        DrawConsoleWindow();
    }

    // Hierarchy Window
    if (showHierarchy) {
        DrawHierarchyWindow();
    }

    // Inspector Window
    if (showInspector) {
        DrawInspectorWindow();
    }

    if (showAbout)
        DrawAboutWindow();

    HandleDeleteKey();

    return true;
}

bool ModuleEditor::PostUpdate()
{
    //ShowMenuBar();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return true;
}

bool ModuleEditor::CleanUp()
{
    LOG_DEBUG("Cleaning up Editor");

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    return true;
}

bool ModuleEditor::ShowMenuBar() {
    if (ImGui::BeginMenuBar()) {

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                Application::GetInstance().RequestExit();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Configuration", NULL, &showConfiguration);
            ImGui::MenuItem("Console", NULL, &showConsole);
            ImGui::MenuItem("Hierarchy", NULL, &showHierarchy);
            ImGui::MenuItem("Inspector", NULL, &showInspector);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObject")) {
            if (ImGui::BeginMenu("Create Primitive")) {
                if (ImGui::MenuItem("Cube")) {
                    CreatePrimitiveGameObject("Cube", Primitives::CreateCube());
                }
                if (ImGui::MenuItem("Pyramid")) {
                    CreatePrimitiveGameObject("Pyramid", Primitives::CreatePyramid());
                }
                if (ImGui::MenuItem("Plane")) {
                    CreatePrimitiveGameObject("Plane", Primitives::CreatePlane());
                }
                if (ImGui::MenuItem("Sphere")) {
                    CreatePrimitiveGameObject("Sphere", Primitives::CreateSphere());
                }
                if (ImGui::MenuItem("Cylinder")) {
                    CreatePrimitiveGameObject("Cylinder", Primitives::CreateCylinder());
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Documentation")) {
                SDL_OpenURL("https://github.com/Audra0000/Engine/blob/main/README.md"); 
            }
            if (ImGui::MenuItem("Report a Bug")) {
                SDL_OpenURL("https://github.com/Audra0000/Engine/issues");
            }
            if (ImGui::MenuItem("Download Latest")) {
                SDL_OpenURL("https://github.com/Audra0000/Engine/releases");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("About")) {
                showAbout = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
    return true;
}

void ModuleEditor::DrawConfigurationWindow()
{
    ImGui::Begin("Configuration", &showConfiguration);

    // FPS Graph
    if (ImGui::CollapsingHeader("FPS")) 
    {
        DrawFPSGraph();
    }

    ImGui::Separator();

    // Window
    if (ImGui::CollapsingHeader("Window"))
    {
        DrawWindowSettings();
    }

    ImGui::Separator();

	// Camera
    if (ImGui::CollapsingHeader("Camera"))
    {
        DrawCameraSettings();
    }

	ImGui::Separator();

	// Renderer
    if (ImGui::CollapsingHeader("Renderer"))
    {
        DrawRendererSettings();
	}

	ImGui::Separator();

    // Hardware
    if (ImGui::CollapsingHeader("Hardware"))
    {
        DrawHardwareInfo();
    }

    ImGui::End();
}

void ModuleEditor::CreatePrimitiveGameObject(const std::string& name, Mesh mesh)
{
    GameObject* Object = new GameObject(name);
    ComponentMesh* meshComp = static_cast<ComponentMesh*>(Object->CreateComponent(ComponentType::MESH));

    // Seleccionar primitiva seg�n el nombre
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

    GameObject* root = Application::GetInstance().scene->GetRoot();
    root->AddChild(Object);

    LOG_CONSOLE("%s created", name.c_str());
    LOG_DEBUG("Primitive created: %s", name.c_str());
}

void ModuleEditor::DrawFPSGraph()
{
    fpsTimer += ImGui::GetIO().DeltaTime;

    if (fpsTimer >= 0.1f)
    {
        float fps = ImGui::GetIO().Framerate;
        fpsHistory.push_back(fps);

        if (fpsHistory.size() > maxFPSHistory)
        {
            fpsHistory.erase(fpsHistory.begin());
        }

        fpsTimer = 0.0f;
    }

    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

    if (!fpsHistory.empty())
    {
		// FPS graph
        ImGui::PlotLines("##FPS", fpsHistory.data(), (int)fpsHistory.size(), 0, nullptr, 0.0f, 200.0f, ImVec2(0, 80));
    }
}

void ModuleEditor::DrawHardwareInfo()
{
    // CPU
    ImGui::Text("CPU Cores: %d", SDL_GetNumLogicalCPUCores());
    // RAM
    ImGui::Text("System RAM: %d MB", SDL_GetSystemRAM());

#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc; // This is used to store memory
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
        // workingSetSize gives used ram in bytes, but we want in mb so we divide it with (1024 * 1024) which is 1,048,576 bytes.
        SIZE_T memoryUsedMB = pmc.WorkingSetSize / (1024 * 1024);
        ImGui::Text("Memory Usage: %llu MB", memoryUsedMB);
    }
#endif

    // GPU
    ImGui::Text("GPU: %s", glGetString(GL_RENDERER));

    // Libraries versions
    ImGui::Separator();
    ImGui::Text("Software Versions:");

    // SDL
    int sdlVersion = SDL_GetVersion();
    int major = SDL_VERSIONNUM_MAJOR(sdlVersion);
    int minor = SDL_VERSIONNUM_MINOR(sdlVersion);
    int patch = SDL_VERSIONNUM_MICRO(sdlVersion);
    ImGui::BulletText("SDL3: %d.%d.%d", major, minor, patch);

	// OpenGL
    ImGui::BulletText("OpenGL: %s", glGetString(GL_VERSION));
	// ImGui
    ImGui::BulletText("ImGui: %s", IMGUI_VERSION);

	// DevIL
    ILint devilVersion = ilGetInteger(IL_VERSION_NUM);
    int devilMajor = devilVersion / 100;
    int devilMinor = (devilVersion / 10) % 10;
    int devilPatch = devilVersion % 10;
    ImGui::BulletText("DevIL: %d.%d.%d", devilMajor, devilMinor, devilPatch);
}

void ModuleEditor::DrawWindowSettings()
{
    int width, height;
    Application::GetInstance().window->GetWindowSize(width, height);

    ImGui::Text("Window Size: %dx%d", width, height);

    // Width slider
    int tempWidth = width;
    if (ImGui::InputInt("Width", &tempWidth, 10, 100))
    {
        if (tempWidth > 0)
        {
            SDL_SetWindowSize(Application::GetInstance().window->GetWindow(), tempWidth, height);
        }
    }

    // Height slider
    int tempHeight = height;
    if (ImGui::InputInt("Height", &tempHeight, 10, 100))
    {
        if (tempHeight > 0)
        {
            SDL_SetWindowSize(Application::GetInstance().window->GetWindow(), width, tempHeight);
        }
    }

    ImGui::Separator();

    // Fullscreen
    if (ImGui::Checkbox("Fullscreen", &fullscreen))
    {
        SDL_SetWindowFullscreen(Application::GetInstance().window->GetWindow(), fullscreen);
    }

    // Borderless
    static bool borderlessFullscreen = false;
    if (ImGui::Checkbox("Borderless Window", &borderlessFullscreen))
    {
        SDL_SetWindowBordered(Application::GetInstance().window->GetWindow(), !borderlessFullscreen);
    }

    ImGui::Separator();

    static bool resizable = true;
    if (ImGui::Checkbox("Resizable", &resizable))
    {
        SDL_SetWindowResizable(Application::GetInstance().window->GetWindow(), resizable);
    }

    ImGui::Separator();

    ImGui::Text("Resolutions:");

    if (ImGui::Button("Set 1920x1080"))
    {
        SDL_SetWindowSize(Application::GetInstance().window->GetWindow(), 1920, 1080);
    }
    ImGui::SameLine();
    if (ImGui::Button("Set 1280x720"))
    {
        SDL_SetWindowSize(Application::GetInstance().window->GetWindow(), 1280, 720);
    }
}

void ModuleEditor::DrawHierarchyWindow()
{
    ImGui::Begin("Hierarchy", &showHierarchy);

    GameObject* root = Application::GetInstance().scene->GetRoot();

    if (root != nullptr)
    {
        DrawGameObjectNode(root);
    }
    else
    {
        ImGui::TextDisabled("No scene loaded");
    }


    ImGui::End();
}

void ModuleEditor::DrawInspectorWindow()
{
    ImGui::Begin("Inspector", &showInspector);

    SelectionManager* selectionManager = Application::GetInstance().selectionManager;

    if (!selectionManager->HasSelection())
    {
        ImGui::TextDisabled("No GameObject selected");
        ImGui::End();
        return;
    }

    GameObject* selectedObject = selectionManager->GetSelectedObject();

    if (selectedObject == nullptr)
    {
        ImGui::TextDisabled("Invalid selection");
        ImGui::End();
        return;
    }

    ImGui::Text("GameObject: %s", selectedObject->GetName().c_str());
    ImGui::Separator();

    // Transforms
    Transform* transform = static_cast<Transform*>(selectedObject->GetComponent(ComponentType::TRANSFORM));

    if (transform != nullptr)
    {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            glm::vec3 position = transform->GetPosition();
            glm::vec3 rotation = transform->GetRotation();
            glm::vec3 scale = transform->GetScale();

            bool transformChanged = false;

            // Position
            ImGui::Text("Position");
            ImGui::PushItemWidth(80);
            ImGui::Text("X"); ImGui::SameLine(20);
            if (ImGui::DragFloat("##PosX", &position.x, 0.1f)) transformChanged = true;
            ImGui::SameLine(120);
            ImGui::Text("Y"); ImGui::SameLine(130);
            if (ImGui::DragFloat("##PosY", &position.y, 0.1f)) transformChanged = true;
            ImGui::SameLine(230);
            ImGui::Text("Z"); ImGui::SameLine(240);
            if (ImGui::DragFloat("##PosZ", &position.z, 0.1f)) transformChanged = true;

            ImGui::Spacing();

            // Rotation
            ImGui::Text("Rotation");
            ImGui::Text("X"); ImGui::SameLine(20);
            if (ImGui::DragFloat("##RotX", &rotation.x, 0.1f)) transformChanged = true;
            ImGui::SameLine(120);
            ImGui::Text("Y"); ImGui::SameLine(130);
            if (ImGui::DragFloat("##RotY", &rotation.y, 0.1f)) transformChanged = true;
            ImGui::SameLine(230);
            ImGui::Text("Z"); ImGui::SameLine(240);
            if (ImGui::DragFloat("##RotZ", &rotation.z, 0.1f)) transformChanged = true;

            ImGui::Spacing();

            // Scale
            ImGui::Text("Scale");
            ImGui::Text("X"); ImGui::SameLine(20);
            if (ImGui::DragFloat("##ScaleX", &scale.x, 0.1f)) transformChanged = true;
            ImGui::SameLine(120);
            ImGui::Text("Y"); ImGui::SameLine(130);
            if (ImGui::DragFloat("##ScaleY", &scale.y, 0.1f)) transformChanged = true;
            ImGui::SameLine(230);
            ImGui::Text("Z"); ImGui::SameLine(240);
            if (ImGui::DragFloat("##ScaleZ", &scale.z, 0.1f)) transformChanged = true;

            ImGui::PopItemWidth();

            if (transformChanged)
            {
                transform->SetPosition(position);
                transform->SetRotation(rotation);
                transform->SetScale(scale);
            }

            ImGui::Spacing();

            if (ImGui::Button("Reset Transform"))
            {
                transform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
                transform->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
                transform->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));

                LOG_DEBUG("Transform reset for: %s", selectedObject->GetName().c_str());
                LOG_CONSOLE("Transform reset for: %s", selectedObject->GetName().c_str());
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Reset position to (0,0,0), rotation to (0,0,0), and scale to (1,1,1)");
            }
        }
    }
    // Mesh
    ComponentMesh* meshComp = static_cast<ComponentMesh*>(selectedObject->GetComponent(ComponentType::MESH));

    if (meshComp != nullptr && meshComp->HasMesh())
    {
        if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const Mesh& mesh = meshComp->GetMesh();

            // Mesh information
            ImGui::Text("Vertices: %d", (int)mesh.vertices.size());
            ImGui::Text("Indices: %d", (int)mesh.indices.size());
            ImGui::Text("Triangles: %d", (int)mesh.indices.size() / 3);

            ImGui::Separator();

            // Normals visualization
            ImGui::Text("Normals Visualization:");

            if (ImGui::Checkbox("Show Vertex Normals", &showVertexNormals))
            {
                LOG_DEBUG("Vertex normals visualization: %s", showVertexNormals ? "ON" : "OFF");
            }

            if (ImGui::Checkbox("Show Face Normals", &showFaceNormals))
            {
                LOG_DEBUG("Face normals visualization: %s", showFaceNormals ? "ON" : "OFF");
            }
        }
    }
    ComponentMaterial* materialComp = static_cast<ComponentMaterial*>(selectedObject->GetComponent(ComponentType::MATERIAL));

    if (materialComp != nullptr)
    {
        if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
        {
			// Texture information
            ImGui::Text("Texture Information:");

            std::string texturePath = materialComp->GetTexturePath();
            if (texturePath.empty() || texturePath == "[Checkerboard Pattern]")
            {
                ImGui::Text("Path: Checkerboard");
            }
            else
            {
                ImGui::Text("Path: %s", texturePath.c_str());
            }

            ImGui::Text("Size: %d x %d pixels", materialComp->GetTextureWidth(), materialComp->GetTextureHeight());

            ImGui::Separator();

            if (materialComp->HasOriginalTexture())
            {
                ImGui::Separator();
                ImGui::Text("Original Texture:");
                ImGui::Text("Path: %s", materialComp->GetOriginalTexturePath().c_str());
            }

            ImGui::Separator();

            ImGui::BeginGroup();

            if (ImGui::Button("Apply Checkerboard"))
            {
                materialComp->CreateCheckerboardTexture();
                LOG_DEBUG("Applied checkerboard texture to: %s", selectedObject->GetName().c_str());
                LOG_CONSOLE("Checkerboard texture applied to %s", selectedObject->GetName().c_str());
            }

			if (ImGui::IsItemHovered()) // Helper tooltip
            {
                ImGui::SetTooltip("Applies the default black and white checkerboard pattern to this GameObject");
            }

            if (materialComp->HasOriginalTexture())
            {
                ImGui::SameLine();

                if (ImGui::Button("Restore Original"))
                {
                    materialComp->RestoreOriginalTexture();
                    LOG_DEBUG("Restored original texture to: %s", selectedObject->GetName().c_str());
                    LOG_CONSOLE("Original texture restored to %s", selectedObject->GetName().c_str());
                }

                if (ImGui::IsItemHovered()) // Helper tooltip
                {
                    ImGui::SetTooltip("Restores the original texture that was previously loaded");
                }
            }

            ImGui::EndGroup();
        }
    }
    else
    {
        if (meshComp != nullptr && meshComp->HasMesh())
        {
            if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("No material component");
            }
        }
    }
    ImGui::End();
}

void ModuleEditor::DrawCameraSettings()
{
    ImGui::Text("Camera Controls");
    ImGui::Separator();

    Camera* camera = Application::GetInstance().renderer->GetCamera();
    if (camera == nullptr)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Camera not available");
        return;
    }

    // Update camera settings
    cameraMouseSensitivity = camera->GetMouseSensitivity();
    cameraScrollSpeed = camera->GetScrollSpeed();
    cameraPanSensitivity = camera->GetPanSensitivity();
    cameraMovementSpeed = camera->GetMovementSpeed();
    cameraFOV = camera->GetFov();

    ImGui::PushItemWidth(80);

    // Movement Speed
    if (ImGui::SliderFloat("Movement Speed", &cameraMovementSpeed, 0.1f, 10.0f, "%.2f"))
    {
        camera->SetMovementSpeed(cameraMovementSpeed);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls how fast the camera moves (WASD keys)");

    // Mouse Sensitivity
    if (ImGui::SliderFloat("Mouse Sensitivity", &cameraMouseSensitivity, 0.01f, 1.0f, "%.3f"))
    {
        camera->SetMouseSensitivity(cameraMouseSensitivity);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls camera rotation sensitivity when right-clicking");

    // Scroll Speed
    if (ImGui::SliderFloat("Scroll Speed", &cameraScrollSpeed, 0.1f, 2.0f, "%.2f"))
    {
        camera->SetScrollSpeed(cameraScrollSpeed);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls zoom speed with mouse wheel");

    // Pan Sensitivity
    if (ImGui::SliderFloat("Pan Sensitivity", &cameraPanSensitivity, 0.001f, 0.01f, "%.4f"))
    {
        camera->SetPanSensitivity(cameraPanSensitivity);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Controls camera panning sensitivity (middle mouse button)");

    ImGui::Spacing();

    // FOV
    if (ImGui::SliderFloat("Field of View", &cameraFOV, 20.0f, 120.0f, "%.1f°"))
    {
        camera->SetFov(cameraFOV);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Camera field of view in degrees");

	ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();

    // Reset button
    if (ImGui::Button("Reset to Defaults"))
    {
        cameraMovementSpeed = 2.5f;
        cameraMouseSensitivity = 0.2f;
        cameraScrollSpeed = 0.5f;
        cameraFOV = 45.0f;
        cameraPanSensitivity = 0.003f;

        camera->SetMovementSpeed(cameraMovementSpeed);
        camera->SetMouseSensitivity(cameraMouseSensitivity);
        camera->SetScrollSpeed(cameraScrollSpeed);
        camera->SetFov(cameraFOV);
        camera->SetPanSensitivity(cameraPanSensitivity);

        LOG_CONSOLE("Camera settings reset to defaults");
        LOG_DEBUG("Camera settings reset to defaults");
    }

    ImGui::Spacing();

    // Camera Position Info
    glm::vec3 camPos = camera->GetPosition();
    ImGui::Text("Camera Position:");
    ImGui::BulletText("X: %.2f  Y: %.2f  Z: %.2f", camPos.x, camPos.y, camPos.z);

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Camera Controls")) {
        ImGui::Text("Camera Controls:");
        ImGui::BulletText("Right Click + Drag: Rotate camera");
        ImGui::BulletText("Middle Click + Drag: Pan camera");
        ImGui::BulletText("Mouse Wheel: Zoom in/out");
        ImGui::BulletText("Alt + Right Click: Orbit around target");
        ImGui::BulletText("F: Focus on selected object");
    }
    ImGui::Separator();
}

void ModuleEditor::DrawRendererSettings()
{
    ImGui::Text("OpenGL Renderer Settings");
    ImGui::Separator();

    Renderer* renderer = Application::GetInstance().renderer.get();
    if (renderer == nullptr)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Renderer not available");
        return;
    }

    // Face Culling
    bool faceCulling = renderer->IsFaceCullingEnabled();
    if (ImGui::Checkbox("Face Culling", &faceCulling))
    {
        renderer->SetFaceCulling(faceCulling);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enable/disable back-face culling");

    // Cull Face Mode
    if (faceCulling)
    {
        ImGui::Indent();
        int cullMode = renderer->GetCullFaceMode();
        const char* cullModes[] = { "Back", "Front", "Front and Back" };
        if (ImGui::Combo("Cull Face Mode", &cullMode, cullModes, 3))
        {
            renderer->SetCullFaceMode(cullMode);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Which faces to cull:\n- Back: Normal (hides back faces)\n- Front: Rare (hides front faces)\n- Both faces");
        ImGui::Unindent();
    }

    ImGui::Spacing();

    // Wireframe Mode
    bool wireframe = renderer->IsWireframeMode();
    if (ImGui::Checkbox("Wireframe Mode", &wireframe))
    {
        renderer->SetWireframeMode(wireframe);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Render meshes as wireframes");

    ImGui::Spacing();
    ImGui::Separator();

    // Background Color
    ImGui::Text("Background Color");

    float currentR, currentG, currentB;
    renderer->GetClearColor(currentR, currentG, currentB);

    static float editColor[3] = { currentR, currentG, currentB };

    // Color picker
    ImGui::ColorEdit3("##ClearColor", editColor, ImGuiColorEditFlags_NoAlpha);

	// Color changed?
    bool colorChanged = (editColor[0] != currentR || editColor[1] != currentG || editColor[2] != currentB);

    ImGui::SameLine();

    // Apply button
    if (!colorChanged)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Apply"))
    {
        renderer->SetClearColor(editColor[0], editColor[1], editColor[2]);
        LOG_CONSOLE("Background color updated");
        LOG_DEBUG("Background color set to (%.2f, %.2f, %.2f)", editColor[0], editColor[1], editColor[2]);
    }

    if (!colorChanged)
    {
        ImGui::EndDisabled();
    }

    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Apply the new background color");

    ImGui::SameLine();

    // Reset Color button
    if (ImGui::Button("Reset Color"))
    {
        editColor[0] = 0.2f;
        editColor[1] = 0.25f;
        editColor[2] = 0.3f;
        renderer->SetClearColor(editColor[0], editColor[1], editColor[2]);
        LOG_CONSOLE("Background color reset to default");
        LOG_DEBUG("Background color reset to default");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset background color to default");

    ImGui::Spacing();
    ImGui::Separator();

    // Reset All button
    if (ImGui::Button("Reset All Settings"))
    {
        renderer->SetFaceCulling(true);
        renderer->SetWireframeMode(false);
        renderer->SetCullFaceMode(0);
        renderer->SetClearColor(0.2f, 0.25f, 0.3f);

        editColor[0] = 0.2f;
        editColor[1] = 0.25f;
        editColor[2] = 0.3f;

        LOG_CONSOLE("All renderer settings reset to defaults");
        LOG_DEBUG("All renderer settings reset to defaults");
    }

    ImGui::Spacing();
}

void ModuleEditor::DrawConsoleWindow()
{
    ImGui::Begin("Console", &showConsole);

    if (ImGui::Button("Clear"))
    {
        ConsoleLog::GetInstance().Clear();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Library Info", &showLibraryInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Errors", &showErrors);
    ImGui::SameLine();
    ImGui::Checkbox("Warnings", &showWarnings);
    ImGui::SameLine();
    ImGui::Checkbox("Loading", &showLoading);
    ImGui::SameLine();
    ImGui::Checkbox("Success", &showSuccess);
    ImGui::SameLine();
    ImGui::Checkbox("Info", &showInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll);

    ImGui::Separator();

    ImVec2 availableSpace = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("Scrolling", availableSpace, true, ImGuiWindowFlags_HorizontalScrollbar);

    const std::vector<std::string>& logs = ConsoleLog::GetInstance().GetLogs();

    for (const auto& log : logs)
    {
        ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
        bool isError = false;
        bool isWarning = false;
        bool isInfo = false;
        bool isSuccess = false;
        bool isLoading = false;
        bool isLibraryInfo = false;

        if (log.find("DevIL") != std::string::npos || log.find("SDL3") != std::string::npos || log.find("OpenGL") != std::string::npos || log.find("ASSIMP") != std::string::npos || log.find("Mesh processed") != std::string::npos)
        {
            color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            isLibraryInfo = true;
        }
        else if (log.find("ERROR") != std::string::npos || log.find("Failed") != std::string::npos)
        {
            color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // Red
            isError = true;
        }
        else if (log.find("WARNING") != std::string::npos || log.find("Corrupt") != std::string::npos)
        {
            color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
            isWarning = true;
        }
        else if (log.find("Loading") != std::string::npos || log.find("Initializing") != std::string::npos)
        {
            color = ImVec4(1.0f, 0.7f, 0.3f, 1.0f); // Orange
            isLoading = true;
        }
        else if (log.find("ready") != std::string::npos || log.find("loaded") != std::string::npos || log.find("initialized") != std::string::npos)
        {
            color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // Green
            isSuccess = true;
        }
        else isInfo = true;

        if (isLibraryInfo)
        {
            if (!showLibraryInfo)
                continue;
        }
        else if ((isError && !showErrors) || (isWarning && !showWarnings) || (isSuccess && !showSuccess) || (isInfo && !showInfo) || (isLoading && !showLoading))
        {
            continue; 
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextWrapped("%s", log.c_str());
        ImGui::PopStyleColor();
    }

    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    if (scrollToBottom)
    {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom = false;
    }

    ImGui::EndChild();
    ImGui::End();
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

    // SDL
    int sdlVersion = SDL_GetVersion();
    int major = SDL_VERSIONNUM_MAJOR(sdlVersion);
    int minor = SDL_VERSIONNUM_MINOR(sdlVersion);
    int patch = SDL_VERSIONNUM_MICRO(sdlVersion);
    ImGui::BulletText("SDL3: %d.%d.%d", major, minor, patch);

    // OpenGL
    ImGui::BulletText("OpenGL: %s", glGetString(GL_VERSION));

    // ImGui
    ImGui::BulletText("ImGui: %s", IMGUI_VERSION);

    // DevIL
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

void ModuleEditor::DrawGameObjectNode(GameObject* gameObject)
{
    if (gameObject == nullptr)
        return;

    const std::vector<GameObject*>& children = gameObject->GetChildren();
    bool hasChildren = !children.empty();

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow; //  | ImGuiTreeNodeFlags_OpenOnDoubleClick

	// Handle selected game object
    SelectionManager* selectionManager = Application::GetInstance().selectionManager;
    if (selectionManager->IsSelected(gameObject))
    {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    if (!hasChildren)
    {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    // =================================================== Handle renaming =====================================================
    bool isRenaming = (renamingObject == gameObject);

    if (isRenaming)
    {
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##", renameBuffer, sizeof(renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))         // Show input text field for renaming
        {
            if (strlen(renameBuffer) > 0)
            {
                gameObject->SetName(renameBuffer);
                LOG_DEBUG("GameObject renamed to: %s", renameBuffer);
            }
            renamingObject = nullptr;
        }

		if (!ImGui::IsItemActive() && (ImGui::IsMouseClicked(0) || ImGui::IsKeyPressed(ImGuiKey_Escape))) // Clicked outside or pressed Escape
        {
            renamingObject = nullptr;
        }

    }
    else
    {
	// ============================================ Display GameObject Node ===================================================
        
        // Checkbox 
        bool isActive = gameObject->IsActive();
        ImGui::PushID(gameObject);
        if (ImGui::Checkbox("##active", &isActive))
        {
            gameObject->SetActive(isActive);
        }
        ImGui::PopID();

        ImGui::SameLine();

        // Display the node
        bool nodeOpen = ImGui::TreeNodeEx(gameObject, nodeFlags, "%s", gameObject->GetName().c_str());
        // Handle selection
        if (ImGui::IsItemClicked())
        {
            const bool* keys = SDL_GetKeyboardState(NULL);
            bool shiftPressed = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

            if (shiftPressed)
            {
                // Shift+ click: Multi select
                selectionManager->ToggleSelection(gameObject);
            }
            else
            {
                if (hasChildren)
                {
                    // Select all children
                    selectionManager->ClearSelection();
                    SelectGameObjectAndChildren(gameObject);
                }
                else
                {
                    // Click 
                    selectionManager->SetSelectedObject(gameObject);
                }
            }
        }

        // Handle renaming on double click
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) // Mouse over and double clicked
        {
            renamingObject = gameObject;
			strncpy(renameBuffer, gameObject->GetName().c_str(), sizeof(renameBuffer) - 1); // Copy current name to buffer
        }

        // Recursively draw children
        if (nodeOpen && hasChildren)
        {
            for (GameObject* child : children)
            {
                DrawGameObjectNode(child);
            }
            ImGui::TreePop();
        }
    }
}

void ModuleEditor::SelectGameObjectAndChildren(GameObject* gameObject)
{
    if (gameObject == nullptr)
        return;

    // Add GameObject to selection
    SelectionManager* selectionManager = Application::GetInstance().selectionManager;
    selectionManager->AddToSelection(gameObject);

    // Recursively add all children
    const std::vector<GameObject*>& children = gameObject->GetChildren();
    for (GameObject* child : children)
    {
        SelectGameObjectAndChildren(child);
    }
}

void ModuleEditor::HandleDeleteKey()
{
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
    {
        if (ImGui::GetIO().WantTextInput) return;

        std::vector<GameObject*> selectedObjects = Application::GetInstance().selectionManager->GetSelectedObjects();

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