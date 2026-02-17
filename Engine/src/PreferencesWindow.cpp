#include "PreferencesWindow.h"
#include "EditorPreferences.h"
#include <imgui.h>

PreferencesWindow::PreferencesWindow()
    : EditorWindow("Preferences")
{
    memset(customEditorPathBuffer, 0, sizeof(customEditorPathBuffer));

    std::string currentPath = EditorPreferences::GetCustomEditorPath();
    if (!currentPath.empty())
    {
        strncpy(customEditorPathBuffer, currentPath.c_str(), sizeof(customEditorPathBuffer) - 1);
    }
}

PreferencesWindow::~PreferencesWindow()
{
}

void PreferencesWindow::Draw()
{
    if (!isOpen) return;

    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin(name.c_str(), &isOpen))
    {
        if (ImGui::BeginTabBar("PreferencesTabs"))
        {
            if (ImGui::BeginTabItem("General"))
            {
                DrawGeneralSettings();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Script Editor"))
            {
                DrawScriptEditorSettings();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void PreferencesWindow::DrawGeneralSettings()
{
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "General Preferences");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Engine version: 1.0");
    ImGui::Text("Project: MyGame");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Reset All Preferences", ImVec2(200, 0)))
    {
        EditorPreferences::SetPreferredEditor(ExternalEditor::INTERNAL);
        EditorPreferences::SetCustomEditorPath("");
    }
}

void PreferencesWindow::DrawScriptEditorSettings()
{
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Script Editor Preferences");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Default Script Editor:");

    ExternalEditor currentEditor = EditorPreferences::GetPreferredEditor();
    const char* editorNames[] = { "Internal Editor", "Visual Studio 2022", "VS Code", "Custom" };
    int currentIdx = static_cast<int>(currentEditor);

    if (ImGui::Combo("##defaulteditor", &currentIdx, editorNames, 4))
    {
        EditorPreferences::SetPreferredEditor(static_cast<ExternalEditor>(currentIdx));
    }

    ImGui::Spacing();

    if (currentEditor == ExternalEditor::INTERNAL)
    {
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
            "Using built-in script editor");
    }
    else if (currentEditor == ExternalEditor::VISUAL_STUDIO_2022)
    {
        std::string vsPath = EditorPreferences::GetEditorExecutablePath();

        if (vsPath.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
                "Visual Studio 2022 not found!");
            ImGui::TextWrapped("Please install VS 2022 or select a different editor.");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
                "Visual Studio 2022 found");
            ImGui::Text("Path: %s", vsPath.c_str());
        }
    }
    else if (currentEditor == ExternalEditor::VSCODE)
    {
        std::string vscodePath = EditorPreferences::GetEditorExecutablePath();

        if (vscodePath.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
                "VS Code not found!");
            ImGui::TextWrapped("Please install VS Code or select a different editor.");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
                "VS Code found");
            ImGui::Text("Path: %s", vscodePath.c_str());
        }
    }
    else if (currentEditor == ExternalEditor::CUSTOM)
    {
        ImGui::Text("Custom Editor Path:");

        if (ImGui::InputText("##customeditorpath", customEditorPathBuffer,
            sizeof(customEditorPathBuffer)))
        {
            EditorPreferences::SetCustomEditorPath(customEditorPathBuffer);
        }

        if (ImGui::Button("Browse...", ImVec2(100, 0)))
        {
            // TODO: Implement file browser
            ImGui::OpenPopup("BrowseNotImplemented");
        }

        if (ImGui::BeginPopupModal("BrowseNotImplemented", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("File browser not yet implemented.");
            ImGui::Text("Please type the full path to your editor executable.");

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
            "Example: C:\\MyEditor\\editor.exe");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Internal Editor Settings");
    ImGui::Spacing();

    static bool showLineNumbers = true;
    static bool autoIndent = true;
    static float fontSize = 14.0f;

    ImGui::Checkbox("Show Line Numbers", &showLineNumbers);
    ImGui::Checkbox("Auto Indent", &autoIndent);
    ImGui::SliderFloat("Font Size", &fontSize, 10.0f, 24.0f, "%.0f");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Tips:");
    ImGui::BulletText("Double-click a .lua file to open it");
    ImGui::BulletText("Right-click for 'Open With' options");
    ImGui::BulletText("Set your preferred editor here to open scripts by default");
}