#ifndef __SCRIPT_EDITOR_WINDOW_H__
#define __SCRIPT_EDITOR_WINDOW_H__

#include "EditorWindow.h"
#include "TextEditor.h" 
#include <string>
#include "ModuleScripting.h"

class ScriptEditorWindow : public EditorWindow {
public:
    ScriptEditorWindow();
    virtual ~ScriptEditorWindow();

    void Draw() override;
    void LoadScript(const std::string& path, ModuleScripting* target);
    void SaveScript();

    bool IsOpen() const { return isOpen; }
    void SetOpen(bool open) { isOpen = open; }
    void ReloadScriptInScene(GameObject* root, const std::string& filePath, bool& errorFound, std::string& errorMsg);

    int ExtractLineFromError(const std::string& error);

public:
    TextEditor editor;
    std::string currentPath;
    std::string currentName;
    std::string currentFilePath;

    ModuleScripting* scripting = nullptr;
    bool showErrorPopup = false; 
    std::string errorMessage;
};

#endif