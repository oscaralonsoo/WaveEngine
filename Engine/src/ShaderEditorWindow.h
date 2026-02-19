#ifndef __SHADER_EDITOR_WINDOW_H__
#define __SHADER_EDITOR_WINDOW_H__

#include "EditorWindow.h"
#include "Globals.h"
#include "ModuleResources.h"
#include <string>

class ResourceShader;

class ShaderEditorWindow : public EditorWindow
{
public:
    ShaderEditorWindow();
    ~ShaderEditorWindow();

    void Draw() override;

private:
    void LoadShaderResource(UID uid);
    void CompileAndSave();

private:
    UID selectedShaderUID = 0;
    std::string compilationError;
    bool showCompilationError = false;

    char shaderEditBuffer[65536]; // Buffer for ImGui input
};

#endif // __SHADER_EDITOR_WINDOW_H__
