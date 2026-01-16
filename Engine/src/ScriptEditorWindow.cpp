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
    lua_State* tempL = luaL_newstate();

  if (luaL_loadstring(tempL, textToSave.c_str()) != LUA_OK)
    {
        const char* err = lua_tostring(tempL, -1);
        errorMessage = err ? err : "Unknown syntax error";
        showErrorPopup = true;

        LOG_CONSOLE("[LUA SYNTAX ERROR] %s", errorMessage.c_str());

        int reportLine = ExtractLineFromError(errorMessage);
        if (reportLine != -1) {
            TextEditor::ErrorMarkers markers;
            
            auto allLines = editor.GetTextLines();
            int realErrorLine = reportLine; 
            int check = reportLine - 2; 
            while (check >= 0) {
                std::string content = allLines[check];
                content.erase(std::remove_if(content.begin(), content.end(), isspace), content.end());
                
                if (!content.empty()) {
                    realErrorLine = check + 1; 
                    break;
                }
                check--;
            }

            markers.insert(std::make_pair(realErrorLine, errorMessage));
            editor.SetErrorMarkers(markers);
        }

        lua_close(tempL);
        return;
    }
    lua_close(tempL);

    showErrorPopup = false;
    errorMessage.clear();
    editor.SetErrorMarkers(TextEditor::ErrorMarkers()); // Limpiar marcadores previos

    if (Application::GetInstance().filesystem->SaveStringToFile(currentPath.c_str(), textToSave)) {
        LOG_CONSOLE("[Editor] Script saved to disk: %s", currentName.c_str());

        bool errorFound = false;
        std::string lastErrorMsg = "";
        GameObject* root = Application::GetInstance().scene->GetRoot();

        // 3. Hot-Reload: Actualizar scripts en la escena
        ReloadScriptInScene(root, currentPath, errorFound, lastErrorMsg);

        if (errorFound) {
            errorMessage = lastErrorMsg;
            showErrorPopup = true;

            int runLine = ExtractLineFromError(errorMessage);
            if (runLine != -1) {
                TextEditor::ErrorMarkers markers;
                markers.insert(std::make_pair(runLine, "Runtime Error: " + errorMessage));
                editor.SetErrorMarkers(markers);
            }
            LOG_CONSOLE("ERROR [Script Hot-Reload] Falló en al menos un objeto.");
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
                LOG_CONSOLE("[SCRIPT ERROR] Object: '%s' | File: '%s' | Error: %s", root->GetName().c_str(), currentName.c_str(), errorMsg.c_str());
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
       /* if(showErrorPopup) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextWrapped("LUA ERROR: %s", errorMessage.c_str());
            ImGui::PopStyleColor();

            if (ImGui::Button("Clear Error")) showErrorPopup = false;
            ImGui::Separator();
        }*/

        if (showErrorPopup) {
            ImGui::BeginChild("ErrorConsole", ImVec2(0, 60), true);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));

            ImGui::BulletText("LUA ERROR:");
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear")) {
                showErrorPopup = false;
                editor.SetErrorMarkers(TextEditor::ErrorMarkers());
            }

            ImGui::TextWrapped("%s", errorMessage.c_str());

            ImGui::PopStyleColor();
            ImGui::EndChild();
            ImGui::Separator();
        }

        editor.Render("TextEditorInstance");
    }
    ImGui::End();
}

int ScriptEditorWindow::ExtractLineFromError(const std::string& error) {
    size_t firstColon = error.find(':');
    if (firstColon != std::string::npos) {
        size_t secondColon = error.find(':', firstColon + 1);
        if (secondColon != std::string::npos) {
            std::string lineStr = error.substr(firstColon + 1, secondColon - firstColon - 1);
            try {
                return std::stoi(lineStr);
            } catch (...) { return -1; }
        }
    }
    return -1;
}