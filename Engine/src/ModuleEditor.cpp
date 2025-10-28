#include "ModuleEditor.h"
#include "Application.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <IL/il.h>
#include <windows.h>
#include <psapi.h>
#include "Log.h"
#include <gl/GL.h>
#include <SDL3/SDL_timer.h>

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
    LOG("Initializing Editor");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; 

    ImGui_ImplSDL3_InitForOpenGL(Application::GetInstance().window->GetWindow(), Application::GetInstance().renderContext->GetContext());
    ImGui_ImplOpenGL3_Init();

    ImGui::StyleColorsDark();

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
    DrawConsoleWindow();

    DrawConfigurationWindow();

    DrawHierarchyWindow();

    DrawInspectorWindow();

    return true;
}

bool ModuleEditor::PostUpdate()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	ShowMenuBar();
	ShowTest();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	return true;
}

bool ModuleEditor::CleanUp()
{
    LOG("Cleaning up Editor");

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    return true;
}

bool ModuleEditor::ShowMenuBar() {
	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Exit")) {
				// Exit the application
				Application::GetInstance().RequestExit();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Console", NULL, &showConsole);
			ImGui::MenuItem("Hierarchy", NULL, &showHierarchy);
			ImGui::MenuItem("Inspector", NULL, &showInspector);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("Documentation")) {
				// Open browser with documentation
				SDL_OpenURL("https://github.com/Audra0000/Engine");
			}
			if (ImGui::MenuItem("Report a Bug")) {
				// Open browser with issue tracker
				SDL_OpenURL("https://github.com/Audra0000/Engine/issues");
			}
			if (ImGui::MenuItem("Download Latest")) {
				// Open browser with latest releases
				SDL_OpenURL("https://github.com/Audra0000/Engine/releases");
			}
			ImGui::Separator();
			ImGui::MenuItem("About", NULL, &showAbout);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
	return true;
}

bool ModuleEditor::ShowTest() {
	float f = 0.5f;
	char buf[256] = "";

    // FPS Graph
    if (ImGui::CollapsingHeader("FPS", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawFPSGraph();
    }

    ImGui::Separator();

    // Window
    if (ImGui::CollapsingHeader("Window", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawWindowSettings();
    }

    ImGui::Separator();

	ImGui::Text("FPS: %.f", fps);
	ImGui::PlotLines("FPS", fpsHistory.data(), fpsHistory.size(), 0, NULL, 0.0f, FLT_MAX, ImVec2(0, 80));
	ImGui::Separator();
	ImGui::PlotHistogram("FPS", fpsHistory.data(), fpsHistory.size(), 0, NULL, 0.0f, FLT_MAX, ImVec2(0, 80));


	if (ImGui::Button("Save")) {
		//etc
	}
	ImGui::InputText("Input", buf, IM_ARRAYSIZE(buf));
	ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
	return true;
}

void ModuleEditor::DrawConfigurationWindow()
{
    ImGui::Begin("Configuration", &showConfiguration);

    // FPS Graph
    if (ImGui::CollapsingHeader("FPS", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawFPSGraph();
    }

    ImGui::Separator();

    // Window
    if (ImGui::CollapsingHeader("Window", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawWindowSettings();
    }

    ImGui::Separator();

    // Hardware
    if (ImGui::CollapsingHeader("Hardware", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawHardwareInfo();
    }

    ImGui::End();
}

void ModuleEditor::DrawFPSGraph()
{
    // Fps
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

    ImGui::End();
}

void ModuleEditor::DrawInspectorWindow()
{
    ImGui::Begin("Inspector", &showInspector);

    ImGui::End();
}

void ModuleEditor::DrawConsoleWindow()
{
    ImGui::Begin("Console", &showConsole);

    ImGui::End();
}