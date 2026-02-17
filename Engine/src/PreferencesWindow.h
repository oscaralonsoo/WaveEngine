#pragma once

#include "EditorWindow.h"
#include <string>

class PreferencesWindow : public EditorWindow
{
public:
    PreferencesWindow();
    ~PreferencesWindow();

    void Draw() override;

private:
    void DrawGeneralSettings();
    void DrawEditorSettings();
    void DrawScriptEditorSettings();

    char customEditorPathBuffer[512];
};