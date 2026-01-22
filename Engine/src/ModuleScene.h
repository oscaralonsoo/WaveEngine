#pragma once
#include "Module.h"
#include "Octree.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <list>

class GameObject;
class FileSystem;
class Renderer;
class ComponentCamera;
class SceneWindow;
class ComponentParticleSystem;

struct FireworkRocket {
    GameObject* go;
    float fuseTimer;
    bool exploded;
    glm::vec3 color;
};

struct FireworkExplosion {
    GameObject* go;
    float lifeTime;
};



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

    // Scene serialization
    bool SaveScene(const std::string& filepath);
    bool LoadScene(const std::string& filepath);
    void ClearScene();

    // Chimney smoke system 
    void CreateChimneySmoke();
    void ConfigureSmokeEffect(ComponentParticleSystem* ps);

    //Fireworks
    void HandleFireworkInput();
    void UpdateFireworksLogic();
    void CreateFirework();
    void CreateExplosion(glm::vec3 Position, glm::vec3 color);

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

    std::list<FireworkRocket> activeRockets;
    std::list<FireworkExplosion> activeExplosions;
};