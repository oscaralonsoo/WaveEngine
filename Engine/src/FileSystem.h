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

// Modern vertex structure
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

// Texture information
struct TextureInfo {
    unsigned int id;
    std::string type;   // e.g., "texture_diffuse", "texture_specular"
    std::string path;
};

// Mesh data container (no OpenGL logic)
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<TextureInfo> textures;

    // OpenGL buffer IDs 
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
};

// Model containing multiple meshes
class Model {
public:
    std::vector<Mesh> meshes;
    std::string directory;

    Model() = default;
    ~Model() = default;
};

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

private:
    GameObject* ProcessNode(aiNode* node, const aiScene* scene, const std::string& directory);
    Mesh ProcessMesh(aiMesh* aiMesh, const aiScene* scene);
    std::vector<TextureInfo> LoadMaterialTextures(aiMaterial* mat, unsigned int type, const std::string& typeName, const std::string& directory);

    int CountNodes(aiNode* node);

    std::string assetsPath;
};
