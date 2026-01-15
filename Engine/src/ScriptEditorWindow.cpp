#include "ScriptEditorWindow.h"
#include "Application.h"
#include "FileSystem.h" 

#include "ModuleScene.h"
#include "GameObject.h"
#include "ModuleScripting.h"

#include <imgui.h>
#include "Log.h"
#include <fstream> 
#include <filesystem>

#define WIN32_LEAN_AND_MEAN // Evita conflictos de "byte" y acelera la compilación
#define NOMINMAX
#include <windows.h> 
#include <shellapi.h>

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
        LOG_CONSOLE("[Editor] Script saved to disk: %s", currentName.c_str());

        std::string syntaxError = scripting->CheckSyntax(currentPath);

        if (!syntaxError.empty())
        {
            errorMessage = syntaxError;
            showErrorPopup = true;
            return; 
        }
        else
        {
            showErrorPopup = false;
            errorMessage.clear();
        }


        bool errorFound = false;
        std::string lastErrorMsg = "";

        GameObject* root = Application::GetInstance().scene->GetRoot();

        ReloadScriptInScene(root, currentPath, errorFound, lastErrorMsg);

        //Errors
        if (errorFound) {
            errorMessage = lastErrorMsg;
            showErrorPopup = true;
            LOG_CONSOLE("ERROR [Script Hot-Reload] Falló la compilación en al menos un objeto.");
        }
        else {
            showErrorPopup = false;
            errorMessage.clear();
        }

    }
    else {
        LOG_CONSOLE("ERROR: Could not save script file to %s", currentPath.c_str());
    }
}
void ScriptEditorWindow::ReloadScriptInScene(GameObject* root, const std::string& filePath, bool& errorFound, std::string& errorMsg)
{
    if (!root) return;

    for (ModuleScripting* script : root->scripts)
    {
      
        if (script->GetFilePath() == filePath)
        {
            if (!script->ReloadScript(filePath))
            {
                errorFound = true;
                errorMsg = script->GetLastError();
                LOG_CONSOLE("ERROR [Hot-Reload] en objeto '%s': %s", root->GetName().c_str(), errorMsg.c_str());
            }
            else
            {
                LOG_CONSOLE("[Hot-Reload] Script actualizado en: %s", root->GetName().c_str());
            }
        }
    }

    // Recursividad para los hijos
    for (GameObject* child : root->GetChildren())
    {
        ReloadScriptInScene(child, filePath, errorFound, errorMsg);
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

            if (ImGui::MenuItem("Open in External Editor")) {
                
                ShellExecute(0, 0, currentPath.c_str(), 0, 0, SW_SHOW);
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
