#include "Octree.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "Frustum.h"
#include "Log.h"
#include <limits>
#include <functional>
#include <glad/glad.h>
#include "ComponentCamera.h"
#include "Shaders.h"
#include "Application.h"
#include <glm/gtc/type_ptr.hpp>

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

        // After removing, check if we should collapse this node
        if (!IsLeaf())
        {
            CollapseIfPossible();
        }

        return true;
    }

    // Try children
    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i)
        {
            if (children[i] != nullptr && children[i]->Remove(obj))
            {
                // After child removed object, check if we should collapse
                CollapseIfPossible();
                return true;
            }
        }
    }

    return false;
}

void OctreeNode::CollapseIfPossible()
{
    if (IsLeaf())
        return;

    // Count total objects in this node and all children
    int totalObjects = objects.size();

    std::function<void(OctreeNode*)> countObjects = [&](OctreeNode* node) {
        if (!node) return;

        totalObjects += node->objects.size();

        if (!node->IsLeaf())
        {
            for (int i = 0; i < 8; ++i)
            {
                if (node->children[i] != nullptr)
                {
                    countObjects(node->children[i]);
                }
            }
        }
        };

    for (int i = 0; i < 8; ++i)
    {
        if (children[i] != nullptr)
        {
            countObjects(children[i]);
        }
    }

    // If total objects is less than max_objects, collapse
    if (totalObjects <= max_objects)
    {
        // Collect all objects from children
        std::vector<GameObject*> allObjects = objects;

        std::function<void(OctreeNode*)> collectObjects = [&](OctreeNode* node) {
            if (!node) return;

            allObjects.insert(allObjects.end(), node->objects.begin(), node->objects.end());

            if (!node->IsLeaf())
            {
                for (int i = 0; i < 8; ++i)
                {
                    if (node->children[i] != nullptr)
                    {
                        collectObjects(node->children[i]);
                    }
                }
            }
            };

        for (int i = 0; i < 8; ++i)
        {
            if (children[i] != nullptr)
            {
                collectObjects(children[i]);
                delete children[i];
                children[i] = nullptr;
            }
        }

        // Move all objects to this node
        objects = allObjects;
    }
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
        if (!obj || !obj->IsActive()) continue;

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

void OctreeNode::DebugDraw() const
{
    // Skip drawing empty leaf nodes
    if (IsLeaf() && objects.empty())
        return;

    // Crear las 8 esquinas del AABB
    glm::vec3 corners[8] = {
        // Bottom face
        glm::vec3(box_min.x, box_min.y, box_min.z), // 0
        glm::vec3(box_max.x, box_min.y, box_min.z), // 1
        glm::vec3(box_max.x, box_min.y, box_max.z), // 2
        glm::vec3(box_min.x, box_min.y, box_max.z), // 3
        // Top face
        glm::vec3(box_min.x, box_max.y, box_min.z), // 4
        glm::vec3(box_max.x, box_max.y, box_min.z), // 5
        glm::vec3(box_max.x, box_max.y, box_max.z), // 6
        glm::vec3(box_min.x, box_max.y, box_max.z)  // 7
    };

    std::vector<float> lineVertices;
    lineVertices.reserve(24 * 3); // 12 edges * 2 points * 3 coords

    // Bottom face edges (4 lines)
    for (int i = 0; i < 4; ++i)
    {
        int next = (i + 1) % 4;
        lineVertices.insert(lineVertices.end(), {
            corners[i].x, corners[i].y, corners[i].z,
            corners[next].x, corners[next].y, corners[next].z
            });
    }

    // Top face edges (4 lines)
    for (int i = 4; i < 8; ++i)
    {
        int next = 4 + ((i - 4 + 1) % 4);
        lineVertices.insert(lineVertices.end(), {
            corners[i].x, corners[i].y, corners[i].z,
            corners[next].x, corners[next].y, corners[next].z
            });
    }

    // Vertical edges (4 lines)
    for (int i = 0; i < 4; ++i)
    {
        lineVertices.insert(lineVertices.end(), {
            corners[i].x, corners[i].y, corners[i].z,
            corners[i + 4].x, corners[i + 4].y, corners[i + 4].z
            });
    }

    // Crear VAO/VBO temporal para las líneas
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float),
        lineVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    // Obtener cámara y renderer
    ComponentCamera* camera = Application::GetInstance().camera->GetActiveCamera();
    if (!camera)
    {
        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        return;
    }

    Renderer* renderer = Application::GetInstance().renderer.get();
    if (!renderer)
    {
        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        return;
    }

    Shader* lineShader = renderer->GetLineShader();

    if (lineShader)
    {
        lineShader->Use();
        GLuint shaderProgram = lineShader->GetProgramID();

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"),
            1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"),
            1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"),
            1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

        // Color based on node depth and occupancy
        float colorFactor = 1.0f - (current_depth / (float)max_depth);
        glm::vec3 color;

        if (!objects.empty())
        {
            // Nodes with objects are more visible
            color = glm::vec3(1.0f, colorFactor * 0.5f + 0.5f, 0.0f); // Orange to yellow
        }
        else
        {
            // Empty nodes are dimmer
            color = glm::vec3(0.3f, colorFactor * 0.3f, 0.0f); // Dark orange
        }

        GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
        if (colorLoc == -1)
            colorLoc = glGetUniformLocation(shaderProgram, "tintColor");

        if (colorLoc != -1)
        {
            glUniform3fv(colorLoc, 1, glm::value_ptr(color));
        }

        glDisable(GL_DEPTH_TEST);

        float lineWidth = !objects.empty() ? 2.0f : 1.0f;
        glLineWidth(lineWidth);
        glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);
        glLineWidth(1.0f);

        glEnable(GL_DEPTH_TEST);
    }

    // Cleanup
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

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

    // Restore default shader
    if (renderer)
    {
        renderer->GetDefaultShader()->Use();
    }
}
