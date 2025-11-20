#include "Octree.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "Frustum.h"
#include "Log.h"
#include <limits>
#include <functional>
#include <glad/glad.h>

bool RayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
    const glm::vec3& aabbMin, const glm::vec3& aabbMax,
    float& distance);

// OctreeNode Implementation

OctreeNode::OctreeNode(const glm::vec3& min, const glm::vec3& max, int maxObjects, int maxDepth, int currentDepth)
    : box_min(min)
    , box_max(max)
    , max_objects(maxObjects)
    , max_depth(maxDepth)
    , current_depth(currentDepth)
{
    for (int i = 0; i < 8; ++i)
    {
        children[i] = nullptr;
    }
}

OctreeNode::~OctreeNode()
{
    Clear();
}

bool OctreeNode::GetObjectWorldAABB(GameObject* obj, glm::vec3& outMin, glm::vec3& outMax)
{
    if (obj == nullptr)
        return false;

    ComponentMesh* mesh = static_cast<ComponentMesh*>(obj->GetComponent(ComponentType::MESH));
    Transform* transform = static_cast<Transform*>(obj->GetComponent(ComponentType::TRANSFORM));

    if (!mesh || !mesh->HasMesh() || !transform)
        return false;

    // Get AABB in local space
    glm::vec3 localMin = mesh->GetAABBMin();
    glm::vec3 localMax = mesh->GetAABBMax();

    // Transform to world space (transform all 8 corners and recalculate AABB)
    glm::mat4 globalMatrix = transform->GetGlobalMatrix();
    glm::vec3 corners[8] = {
        glm::vec3(globalMatrix * glm::vec4(localMin.x, localMin.y, localMin.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMax.x, localMin.y, localMin.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMin.x, localMax.y, localMin.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMax.x, localMax.y, localMin.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMin.x, localMin.y, localMax.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMax.x, localMin.y, localMax.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMin.x, localMax.y, localMax.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMax.x, localMax.y, localMax.z, 1.0f))
    };

    outMin = corners[0];
    outMax = corners[0];
    for (int i = 1; i < 8; ++i)
    {
        outMin = glm::min(outMin, corners[i]);
        outMax = glm::max(outMax, corners[i]);
    }

    return true;
}

void OctreeNode::Clear()
{
    // Clear all children recursively
    for (int i = 0; i < 8; ++i)
    {
        if (children[i] != nullptr)
        {
            delete children[i];
            children[i] = nullptr;
        }
    }

    // Clear objects list
    objects.clear();
}

bool OctreeNode::Insert(GameObject* obj)
{
    if (obj == nullptr)
        return false;

    // Get object's AABB in world space
    glm::vec3 worldMin, worldMax;
    if (!GetObjectWorldAABB(obj, worldMin, worldMax))
        return false;

    // Check if object is completely outside this node
    if (worldMax.x < box_min.x || worldMin.x > box_max.x ||
        worldMax.y < box_min.y || worldMin.y > box_max.y ||
        worldMax.z < box_min.z || worldMin.z > box_max.z)
    {
        return false; // Object is outside this node
    }

    // If we're a leaf and have space, add to this node
    if (IsLeaf() && objects.size() < static_cast<size_t>(max_objects))
    {
        objects.push_back(obj);
        return true;
    }

    // If we're a leaf but full, subdivide
    if (IsLeaf() && current_depth < max_depth)
    {
        Subdivide();
        RedistributeObjects();
    }

    // If we have children, try to add to appropriate child(ren)
    if (!IsLeaf())
    {
        bool added = false;
        for (int i = 0; i < 8; ++i)
        {
            if (children[i] != nullptr && children[i]->Insert(obj))
            {
                added = true;
            }
        }

        // If object doesn't fit completely in any child, keep it in this node
        if (!added)
        {
            objects.push_back(obj);
            return true;
        }

        return added;
    }

    // We're at max depth and full, add anyway
    objects.push_back(obj);
    return true;
}

bool OctreeNode::Remove(GameObject* obj)
{
    if (obj == nullptr)
        return false;

    // Remove from this node
    auto it = std::find(objects.begin(), objects.end(), obj);
    if (it != objects.end())
    {
        objects.erase(it);
        return true;
    }

    // Try children
    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i] != nullptr && children[i]->Remove(obj))
            {
                return true;
            }
        }
    }

    return false;
}

void OctreeNode::Subdivide()
{
    glm::vec3 center = (box_min + box_max) * 0.5f;

    // Create 8 children
    // Bottom 4 (lower half in Y)
    children[0] = new OctreeNode(
        glm::vec3(box_min.x, box_min.y, box_min.z),
        glm::vec3(center.x, center.y, center.z),
        max_objects, max_depth, current_depth + 1
    );

    children[1] = new OctreeNode(
        glm::vec3(center.x, box_min.y, box_min.z),
        glm::vec3(box_max.x, center.y, center.z),
        max_objects, max_depth, current_depth + 1
    );

    children[2] = new OctreeNode(
        glm::vec3(box_min.x, box_min.y, center.z),
        glm::vec3(center.x, center.y, box_max.z),
        max_objects, max_depth, current_depth + 1
    );

    children[3] = new OctreeNode(
        glm::vec3(center.x, box_min.y, center.z),
        glm::vec3(box_max.x, center.y, box_max.z),
        max_objects, max_depth, current_depth + 1
    );

    // Top 4 (upper half in Y)
    children[4] = new OctreeNode(
        glm::vec3(box_min.x, center.y, box_min.z),
        glm::vec3(center.x, box_max.y, center.z),
        max_objects, max_depth, current_depth + 1
    );

    children[5] = new OctreeNode(
        glm::vec3(center.x, center.y, box_min.z),
        glm::vec3(box_max.x, box_max.y, center.z),
        max_objects, max_depth, current_depth + 1
    );

    children[6] = new OctreeNode(
        glm::vec3(box_min.x, center.y, center.z),
        glm::vec3(center.x, box_max.y, box_max.z),
        max_objects, max_depth, current_depth + 1
    );

    children[7] = new OctreeNode(
        glm::vec3(center.x, center.y, center.z),
        glm::vec3(box_max.x, box_max.y, box_max.z),
        max_objects, max_depth, current_depth + 1
    );
}

void OctreeNode::RedistributeObjects()
{
    // Keep current objects
    std::vector<GameObject*> objectsToRedistribute = objects;
    objects.clear();

    // Try to insert each object into children
    for (GameObject* obj : objectsToRedistribute)
    {
        bool added = false;

        // Try to add to children
        for (int i = 0; i < 8; ++i)
        {
            if (children[i] != nullptr && children[i]->Insert(obj))
            {
                added = true;
                break; // Object added to a child
            }
        }

        // If object doesn't fit in any child, keep it in this node
        if (!added)
        {
            objects.push_back(obj);
        }
    }
}

void OctreeNode::DebugDraw() const
{
    // TODO: Draw AABB for this node

    // Recursively draw children
    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i] != nullptr)
            {
                children[i]->DebugDraw();
            }
        }
    }
}

// Octree Implementation

Octree::Octree()
    : root(nullptr)
{
}

Octree::Octree(const glm::vec3& min, const glm::vec3& max, int maxObjects, int maxDepth)
    : root(nullptr)
{
    Create(min, max, maxObjects, maxDepth);
}

Octree::~Octree()
{
    Clear();
}

void Octree::Create(const glm::vec3& min, const glm::vec3& max, int maxObjects, int maxDepth)
{
    Clear();
    root = new OctreeNode(min, max, maxObjects, maxDepth, 0);
}

void Octree::Clear()
{
    if (root != nullptr)
    {
        delete root;
        root = nullptr;
    }
}

bool Octree::Insert(GameObject* obj)
{
    if (root == nullptr)
        return false;

    return root->Insert(obj);
}

bool Octree::Remove(GameObject* obj)
{
    if (root == nullptr)
        return false;

    return root->Remove(obj);
}

void Octree::DebugDraw() const
{
    if (root != nullptr)
    {
        root->DebugDraw();
    }
}

GameObject* Octree::RayPick(const Ray& ray, float& outDistance) const
{
    if (root == nullptr)
        return nullptr;

    return root->RayPick(ray, outDistance);
}

int Octree::GetTotalObjectCount() const
{
    if (root == nullptr)
        return 0;

    int count = root->GetObjectCount();

    std::function<void(const OctreeNode*)> countRecursive = [&](const OctreeNode* node) {
        if (node->HasChildren())
        {
            for (int i = 0; i < 8; ++i)
            {
                if (node->children[i] != nullptr)
                {
                    count += node->children[i]->GetObjectCount();
                    countRecursive(node->children[i]);
                }
            }
        }
        };

    countRecursive(root);
    return count;
}

int Octree::GetTotalNodeCount() const
{
    if (root == nullptr)
        return 0;

    int count = 1;

    std::function<void(const OctreeNode*)> countRecursive = [&](const OctreeNode* node) {
        if (node->HasChildren())
        {
            for (int i = 0; i < 8; ++i)
            {
                if (node->children[i] != nullptr)
                {
                    count++;
                    countRecursive(node->children[i]);
                }
            }
        }
        };

    countRecursive(root);
    return count;
}

GameObject* OctreeNode::RayPick(const Ray& ray, float& outDistance) const
{
    // Check if ray intersects this node's AABB
    float distance;
    if (!RayIntersectsAABB(ray.origin, ray.direction, box_min, box_max, distance))
    {
        return nullptr;
    }

    GameObject* closestObject = nullptr;
    float closestDistance = std::numeric_limits<float>::max();

    // Check objects in this node
    for (GameObject* obj : objects)
    {
        glm::vec3 worldMin, worldMax;
        if (GetObjectWorldAABB(obj, worldMin, worldMax))
        {
            float objDistance;
            if (RayIntersectsAABB(ray.origin, ray.direction, worldMin, worldMax, objDistance))
            {
                if (objDistance < closestDistance)
                {
                    closestDistance = objDistance;
                    closestObject = obj;
                }
            }
        }
    }

    // Check children recursively
    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i] != nullptr)
            {
                float childDistance;
                GameObject* childResult = children[i]->RayPick(ray, childDistance);
                if (childResult != nullptr && childDistance < closestDistance)
                {
                    closestDistance = childDistance;
                    closestObject = childResult;
                }
            }
        }
    }

    outDistance = closestDistance;
    return closestObject;
}

int OctreeNode::GetObjectCount() const
{
    return static_cast<int>(objects.size());
}