#ifndef __SCRIPT_EDITOR_WINDOW_H__
#define __SCRIPT_EDITOR_WINDOW_H__

#include "EditorWindow.h"
#include "TextEditor.h" 
#include <string>

class ScriptEditorWindow : public EditorWindow {
public:
    ScriptEditorWindow();
    virtual ~ScriptEditorWindow();

    void Draw() override;
    void LoadScript(const std::string& path);
    void SaveScript();

    void OpenScript(const std::string& path);

    bool IsOpen() const { return isOpen; }
    void SetOpen(bool open) { isOpen = open; }

public:
    TextEditor editor;
    std::string currentPath;
    std::string currentName;
    std::string currentFilePath;
};

#endif