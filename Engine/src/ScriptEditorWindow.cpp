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

    editor.SetReadOnly(false);

    isOpen = false;

}

ScriptEditorWindow::~ScriptEditorWindow() {}

void ScriptEditorWindow::LoadScript(const std::string& path) {
    currentPath = path;
    currentName = path.substr(path.find_last_of("/\\") + 1);

    std::string content = Application::GetInstance().filesystem->LoadFileToString(path.c_str());

    if (!content.empty() || content == "") {
        editor.SetText(content);
        editor.SetReadOnly(false);

        isOpen = true; 
        ImGui::SetWindowFocus("Script Editor");
    }
}

void ScriptEditorWindow::SaveScript() {
    if (currentPath.empty()) return;

    std::string textToSave = editor.GetText();

    if (Application::GetInstance().filesystem->SaveStringToFile(currentPath.c_str(), textToSave)) {
        LOG_CONSOLE("[Editor] Script saved: %s", currentName.c_str());
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
        editor.Render("TextEditorInstance");
    }
    ImGui::End();
}
void ScriptEditorWindow::OpenScript(const std::string& path) {
    std::ifstream t(path);
    if (t.good()) {
        std::string str((std::istreambuf_iterator<char>(t)),
            std::istreambuf_iterator<char>());

        editor.SetText(str);
        this->currentFilePath = path; // Guarda la ruta para poder guardar luego (Ctrl+S)
        this->name = "Script Editor - " + std::filesystem::path(path).filename().string();
    }
}