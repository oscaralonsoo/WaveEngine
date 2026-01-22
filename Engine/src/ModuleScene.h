#pragma once
#include "Module.h"
#include "Octree.h"
#include <memory>
#include <vector>

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

    GameObject* CreateCube();

    GameObject* CreateStaticCube();
    
    GameObject* CreateSphere();

    void CleanupMarkedObjects(GameObject* parent);

    Octree* GetOctree() { return octree.get(); }
    void RebuildOctree();
    void MarkOctreeForRebuild() { needsOctreeRebuild = true; }

    // Scene serialization
    bool SaveScene(const std::string& filepath);
    bool LoadScene(const std::string& filepath);
    void ClearScene();
    void ApplyPhysicsToAll(GameObject* obj);

    // for raycast visualization
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
};