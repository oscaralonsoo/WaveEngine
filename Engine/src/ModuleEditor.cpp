#include "ModuleEditor.h"
#include "Application.h"
#include "Log.h"
#include <gl/GL.h>
#include <SDL3/SDL_timer.h>

ModuleEditor::ModuleEditor() {
	name = "ModuleEditor";
}

ModuleEditor::~ModuleEditor() {

}

bool ModuleEditor::Awake() {
	return true;
}

bool ModuleEditor::Start() {
	LOG("Initializing Editor");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//Imgui::StyleColorsClassic();

	ImGui_ImplSDL3_InitForOpenGL(Application::GetInstance().window->GetWindow(), Application::GetInstance().renderContext->GetContext());
	ImGui_ImplOpenGL3_Init("#version 330");

	LOG("ImGui initialized");

	return true;
}

bool ModuleEditor::Update() {
	return true;
}

bool ModuleEditor::PostUpdate() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ShowTest();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	return true;
}

bool ModuleEditor::CleanUp() {
	LOG("Cleaning up Editor");

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	return true;
}

bool ModuleEditor::ShowTest() {
	float f = 0.5f;
	char buf[256] = "";

	static std::vector<float> fpsHistory;

	float fps = ImGui::GetIO().Framerate;

	fpsHistory.push_back(fps);

	if (fpsHistory.size() > 100) {
		fpsHistory.erase(fpsHistory.begin());
	}

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