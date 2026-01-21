#pragma once
#include "Module.h"
#include "Octree.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

class GameObject;
class FileSystem;
class Renderer;
class ComponentCamera;
class SceneWindow;

class ModuleScene : public Module
{
public:
    ModuleScene();
    virtual ~ModuleScene();

    bool Awake() override;
    bool Start() override;
    bool Update() override;
    bool PostUpdate() override;
    bool CleanUp() override;

    GameObject* CreateGameObject(const std::string& name);
    GameObject* GetRoot() const { return root; }
    void CleanupMarkedObjects(GameObject* parent);

    Octree* GetOctree() { return octree.get(); }
    void RebuildOctree();
    void MarkOctreeForRebuild() { needsOctreeRebuild = true; }

    bool SaveScene(const std::string& filepath);
    bool LoadScene(const std::string& filepath);
    void ClearScene();

    void StartAudio3D();
    void StopAudio3D();

    glm::vec3 lastRayOrigin = glm::vec3(0.0f);
    glm::vec3 lastRayDirection = glm::vec3(0.0f);
    float lastRayLength = 0.0f;

private:
    std::unique_ptr<Octree> octree;
    bool needsOctreeRebuild = false;

    GameObject* root = nullptr;
    Renderer* renderer = nullptr;
    FileSystem* filesystem = nullptr;

    ComponentCamera* FindCameraInHierarchy(GameObject* obj);

    GameObject* listenerObject = nullptr;
    GameObject* staticAudioObject = nullptr;
    GameObject* dynamicAudioObject = nullptr;

    bool sfxEnabled = true;
    bool sfxStarted = false;
    unsigned int staticPlayingID = 0;
    unsigned int dynamicPlayingID = 0;

    glm::vec3 tunnelCenter = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 tunnelHalfSize = glm::vec3(2.0f, 2.0f, 2.0f);

    float tunnelAmount = 0.0f;
    float tunnelTarget = 0.0f;
    bool  tunnelInside = false;

    bool tunnelForce = false;
    float tunnelForceValue = 0.0f;
};
