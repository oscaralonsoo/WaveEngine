#pragma once

#include "Module.h"
#include <iostream>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "ModuleResources.h"  

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
class ModuleLoader : public Module
{
public:
    ModuleLoader();
    ~ModuleLoader();

    bool Awake() override;
    bool Start() override;
    bool Update() override;
    bool CleanUp() override;
    
    bool LoadTextureToGameObject(GameObject* obj, const std::string& texturePath);
    bool LoadFbx(const std::string& assetPath);

private:
    
};