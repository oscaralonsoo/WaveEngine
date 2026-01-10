#include "ScriptEditorWindow.h"
#include "Application.h"
#include "FileSystem.h" 
#include <imgui.h>
#include "Log.h"
#include <fstream> 
#include <filesystem>


ScriptEditorWindow::ScriptEditorWindow() : EditorWindow("Script Editor") {
    auto lang = TextEditor::LanguageDefinition::Lua();
    editor.SetLanguageDefinition(lang);
    editor.SetPalette(TextEditor::GetDarkPalette());
    editor.SetShowWhitespaces(false); //Quita los puntos
    editor.SetReadOnly(false);

    isOpen = false;

}

ScriptEditorWindow::~ScriptEditorWindow() {}

void ScriptEditorWindow::LoadScript(const std::string& path, ModuleScripting* target) {
    currentPath = path;
    currentName = std::filesystem::path(path).filename().string();
    scripting = target;

    std::string content = Application::GetInstance().filesystem->LoadFileToString(path.c_str());

    editor.SetText(content);
    editor.SetReadOnly(false);

    isOpen = true; 
    ImGui::SetWindowFocus("Script Editor");
    

}
void ScriptEditorWindow::SaveScript() {
    if (currentPath.empty()) return;

    std::string textToSave = editor.GetText();

    if (Application::GetInstance().filesystem->SaveStringToFile(currentPath.c_str(), textToSave)) {
        LOG_CONSOLE("[Editor] Script saved: %s", currentName.c_str());

        if (scripting) {
            if (!scripting->ReloadScript(currentPath)) {
                errorMessage = scripting->GetLastError();
                showErrorPopup = true;
                LOG_CONSOLE("ERROR [Script Hot-Reload] %s", errorMessage.c_str());
            }
            else {
                showErrorPopup = false; 
            }
        }

    }
}

void ScriptEditorWindow::Draw() {
    if (!isOpen) return;

    // Atajo Ctrl + S
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        SaveScript();
    }

    std::string title = "Script Editor - " + currentName + "###ScriptEditor";

    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin(title.c_str(), &isOpen, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar)) {

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save", "Ctrl+S")) SaveScript();
                if (ImGui::MenuItem("Close")) isOpen = false;
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        if(showErrorPopup) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("LUA ERROR: %s", errorMessage.c_str());
            ImGui::PopStyleColor();

            if (ImGui::Button("Clear Error")) showErrorPopup = false;
            ImGui::Separator();
        }

        editor.Render("TextEditorInstance");
    }
    ImGui::End();
}
