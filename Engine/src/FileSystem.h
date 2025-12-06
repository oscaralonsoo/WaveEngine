#pragma once

#include "Module.h"
#include <iostream>
#include <vector>
#include <string>
#include <glm/glm.hpp>

class GameObject;
struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;
struct MetaFile;

// Vertex data structure
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

// Texture information
struct TextureInfo {
    unsigned int id = 0;
    std::string type;  // e.g., "diffuse", "specular", "normal"
    std::string path;
};

// Mesh container with vertex data and OpenGL buffer IDs
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<TextureInfo> textures;

    // OpenGL buffer IDs 
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    bool IsValid() const { return VAO != 0; }
};

// FBX/Model loading and management
class FileSystem : public Module
{
public:
    FileSystem();
    ~FileSystem();

    bool Awake() override;
    bool Start() override;
    bool Update() override;
    bool CleanUp() override;

    // Loads an FBX file and converts it into a GameObject hierarchy
    GameObject* LoadFBXAsGameObject(const std::string& file_path);

    // Apply texture to a GameObject and its children
    bool ApplyTextureToGameObject(GameObject* obj, const std::string& texturePath);

private:
    // Recursively process scene nodes
    GameObject* ProcessNode(aiNode* node, const aiScene* scene, const std::string& directory);

    // Convert Assimp mesh to engine mesh format
    Mesh ProcessMesh(aiMesh* aiMesh, const aiScene* scene);

    // Scale model to fit target size
    void NormalizeModelScale(GameObject* rootObject, float targetSize);

    // Calculate world-space bounding box
    void CalculateBoundingBox(GameObject* obj, glm::vec3& minBounds, glm::vec3& maxBounds, const glm::mat4& parentTransform);
};