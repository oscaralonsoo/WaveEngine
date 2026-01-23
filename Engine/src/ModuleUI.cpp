#include "ModuleUI.h"
#include "Application.h"
#include "ModuleResources.h"
#include "Texture.h"
#include "Input.h"
#include "Window.h"
#include "Time.h"
#include "Log.h"
#include <imgui.h>
bool ModuleUI::colour = true;

ModuleUI::ModuleUI() : Module()
{
	name = "ModuleUI";
}

ModuleUI::~ModuleUI()
{
}

bool ModuleUI::Start()
{
	LOG_DEBUG("Initializing Module UI");
	
	return true;
}

bool ModuleUI::Update()
{

	if (Application::GetInstance().input->GetKey(SDL_SCANCODE_F1) == KEY_DOWN)
		showOptions = !showOptions;

	switch (uiState)
	{
	case UIState::MAIN_MENU:
		RenderMainMenu();
		break;

	case UIState::FADING_OUT:

		fadeAlpha -= fadeSpeed * Application::GetInstance().time->GetRealDeltaTime();
		if (fadeAlpha < 0.0f) fadeAlpha = 0.0f;

		
		
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, (float)fadeAlpha);

		RenderMainMenu();

		ImGui::PopStyleVar(); 

		if (fadeAlpha <= 0.0f)
		{
			uiState = UIState::IN_GAME;
			fadeAlpha = 0.0f; 
		}
		break;

	case UIState::IN_GAME:
		RenderHUD();
		if (showOptions) RenderOptionsWindow();
		break;
	}

	return true;
}

void ModuleUI::RenderMainMenu()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::Begin("MainMenu", nullptr, flags);

	if (mainMenuBackground)
	{
		ImGui::SetCursorPos(ImVec2(0, 0));
		ImGui::Image((ImTextureID)(unsigned long long)mainMenuBackground->GetID(), viewport->Size);
	}
	else
	{
		ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), viewport->Size, IM_COL32(20, 30, 50, 255));
	}


	ImVec2 center = ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f);

	ImGui::SetCursorPos(ImVec2(center.x - 80, center.y - 100));
	ImGui::SetWindowFontScale(2.0f);
	ImGui::Text("WaveEngine UI");
	ImGui::SetWindowFontScale(1.0f);


	ImGui::SetCursorPos(ImVec2(center.x - 100, center.y));
	ImGui::Text("Nombre del Jugador:");
	ImGui::SetCursorPosX(center.x - 100);
	ImGui::SetNextItemWidth(200.0f);
	ImGui::InputText("##NameInput", playerNameInput, sizeof(playerNameInput));


	ImGui::SetCursorPos(ImVec2(center.x - 60, center.y + 60));
	if (ImGui::Button("Jugar", ImVec2(120, 40)))
	{
		StartGame();
	}

	ImGui::End();
}

void ModuleUI::StartGame()
{
	uiState = UIState::FADING_OUT;
	fadeAlpha = 1.0f;
	LOG_CONSOLE("Inicio: %s", playerNameInput);
}

void ModuleUI::RenderOptionsWindow()
{
	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Game Options (F1)", &showOptions))
	{
		ImGui::Text("Settings");
		ImGui::Separator();

		if (ImGui::Checkbox("Vertical Sync", &vsyncEnabled))
		{
			Application::GetInstance().window->SetVsync(vsyncEnabled);
		}
		if (ImGui::Checkbox("Cross colour", &colour))
		{
			
		}
	
		ImGui::Spacing();
		ImGui::Text("Player: %s", playerNameInput);

		if (ImGui::Button("Close"))
		{
			showOptions = false;
		}
	}
	ImGui::End();
}


void ModuleUI::RenderHUD()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 center = ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f);

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
	ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBringToFrontOnFocus);

	float size = 10.0f;
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	drawList->AddLine(ImVec2(center.x - size, center.y), ImVec2(center.x + size, center.y), IM_COL32(255, 255, 255, 255), 2.0f);
	drawList->AddLine(ImVec2(center.x, center.y - size), ImVec2(center.x, center.y + size), IM_COL32(255, 255, 255, 255), 2.0f);

	ImGui::End();
	ImGui::PopStyleColor();
}
void ModuleUI::DrawCrosshairInsideWindow() {
	//ImGui::Begin("crosstest");
	ImVec2 center = ImVec2(
		ImGui::GetWindowPos().x + ImGui::GetWindowSize().x * 0.5f,
		ImGui::GetWindowPos().y + ImGui::GetWindowSize().y * 0.5f
	);
	float lineSize = 10.0f;
	float gap = 3.0f;       
	float thickness = 2.0f;
	ImU32 color = IM_COL32(0, 255, 0, 255);

	
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	if (colour==true) {
		drawList->AddLine(ImVec2(center.x, center.y - gap), ImVec2(center.x, center.y - gap - lineSize), IM_COL32(0, 255, 255, 255), thickness);

		drawList->AddLine(ImVec2(center.x, center.y + gap), ImVec2(center.x, center.y + gap + lineSize), IM_COL32(0, 255,255, 255), thickness);

		drawList->AddLine(ImVec2(center.x - gap, center.y), ImVec2(center.x - gap - lineSize, center.y), IM_COL32(0, 255, 255, 255), thickness);

		drawList->AddLine(ImVec2(center.x + gap, center.y), ImVec2(center.x + gap + lineSize, center.y), IM_COL32(0, 255, 255, 255), thickness);
	}
	else {
	drawList->AddLine(ImVec2(center.x, center.y - gap), ImVec2(center.x, center.y - gap - lineSize), color, thickness);
	
	drawList->AddLine(ImVec2(center.x, center.y + gap), ImVec2(center.x, center.y + gap + lineSize), color, thickness);
	
	drawList->AddLine(ImVec2(center.x - gap, center.y), ImVec2(center.x - gap - lineSize, center.y), color, thickness);
	
	drawList->AddLine(ImVec2(center.x + gap, center.y), ImVec2(center.x + gap + lineSize, center.y), color, thickness);
	}
	
	
	ImGui::GetWindowDrawList()->AddCircleFilled(center, 2.0f, IM_COL32(255, 0, 0, 255));


}