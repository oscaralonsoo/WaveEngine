#pragma once
#include "Module.h"
#include "Octree.h"
#include <memory>
class GameObject;
class FileSystem;
class Renderer;

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
    void UpdateObjectInOctree(GameObject* obj);

    // Para visualización de raycast
    glm::vec3 lastRayOrigin;
    glm::vec3 lastRayDirection;
    float lastRayLength = 0.0f;

private:

    std::unique_ptr<Octree> octree;
    bool needsOctreeRebuild = false;
    GameObject* root = nullptr;

	Renderer* renderer = nullptr;
	FileSystem* filesystem = nullptr;
};

