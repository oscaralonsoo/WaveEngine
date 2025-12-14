#pragma once

#include <glm/glm.hpp>
#include <vector>

class GameObject;
class Frustum;

struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;

    Ray(const glm::vec3& o, const glm::vec3& d) : origin(o), direction(d) {}
};

class OctreeNode
{
public:
    OctreeNode(const glm::vec3& min, const glm::vec3& max, int maxObjects = 4, int maxDepth = 5, int currentDepth = 0);
    ~OctreeNode();

    void Clear();
    bool Insert(GameObject* obj);
    bool Remove(GameObject* obj);

    template<typename TYPE>
    void CollectIntersections(std::vector<GameObject*>& objects, const TYPE& primitive) const;

    // New methods needed by Octree
    GameObject* RayPick(const Ray& ray, float& outDistance) const;
    int GetObjectCount() const;
    bool HasChildren() const { return children[0] != nullptr; }

    // Debug
    void DebugDraw() const;
    const glm::vec3& GetMin() const { return box_min; }
    const glm::vec3& GetMax() const { return box_max; }

private:
    void Subdivide();
    void RedistributeObjects();
    void CollapseIfPossible(); // Collapse node if it has few objects
    bool IsLeaf() const { return children[0] == nullptr; }

    // Helper to get world-space AABB of a GameObject
    static bool GetObjectWorldAABB(GameObject* obj, glm::vec3& outMin, glm::vec3& outMax);

    // Grant Octree access to private members for counting
    friend class Octree;

private:
    glm::vec3 box_min;
    glm::vec3 box_max;

    std::vector<GameObject*> objects;
    OctreeNode* children[8];  // 8 children for octree

    int max_objects;    // Max objects before subdividing
    int max_depth;      // Max depth of tree
    int current_depth;  // Current depth level
};

class Octree
{
public:
    Octree();
    Octree(const glm::vec3& min, const glm::vec3& max, int maxObjects = 4, int maxDepth = 5);
    ~Octree();

    void Create(const glm::vec3& min, const glm::vec3& max, int maxObjects = 4, int maxDepth = 5);
    void Clear();
    bool Insert(GameObject* obj);
    bool Remove(GameObject* obj);

    template<typename TYPE>
    void CollectIntersections(std::vector<GameObject*>& objects, const TYPE& primitive) const;

    // Ray picking
    GameObject* RayPick(const Ray& ray, float& outDistance) const;

    // Statistics
    int GetTotalObjectCount() const;
    int GetTotalNodeCount() const;

    // Debug
    void DebugDraw() const;

private:
    OctreeNode* root;
};


#include "Frustum.h"

template<typename TYPE>
void OctreeNode::CollectIntersections(std::vector<GameObject*>& objects_out, const TYPE& primitive) const
{
}

// Specialization for Frustum
template<>
inline void OctreeNode::CollectIntersections(std::vector<GameObject*>& objects_out, const Frustum& frustum) const
{
    // Test if this node's AABB intersects with frustum
    FrustumTestResult result = frustum.ContainsAABB(box_min, box_max);

    if (result == FRUSTUM_OUT)
    {
        return; // Node completely outside frustum, skip
    }

    // Add all objects in this node
    for (GameObject* obj : objects)
    {
        objects_out.push_back(obj);
    }

    // Check children recursively
    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i] != nullptr)
            {
                children[i]->CollectIntersections(objects_out, frustum);
            }
        }
    }
}

template<typename TYPE>
void Octree::CollectIntersections(std::vector<GameObject*>& objects, const TYPE& primitive) const
{
    if (root != nullptr)
    {
        root->CollectIntersections(objects, primitive);
    }
}