#pragma once
#include "Module.h"
#include "ComponentCamera.h"
#include <memory>

class GameObject;

class ModuleCamera : public Module
{
public:
    ModuleCamera();
    ~ModuleCamera();

    bool Start() override;
    bool Update() override;
    bool CleanUp() override;

    ComponentCamera* GetActiveCamera() const;

    ComponentCamera* GetEditorCamera() const { return editorCamera; }
    ComponentCamera* GetSceneCamera() const { return sceneCamera; }

    void SetSceneCamera(ComponentCamera* camera) { sceneCamera = camera; }

	// Switch between editor and scene camera
    void SetUseEditorCamera(bool useEditor) { useEditorCamera = useEditor; }
    bool IsUsingEditorCamera() const { return useEditorCamera; }

    // Handle editor camera controls
    void HandleEditorControls();

private:
    GameObject* editorCameraGO;    
    ComponentCamera* editorCamera;   
    ComponentCamera* sceneCamera; 

    bool useEditorCamera;      
};

