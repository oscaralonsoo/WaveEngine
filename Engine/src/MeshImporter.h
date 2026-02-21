#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "ResourceMesh.h"

// Forward declarations
struct Mesh;
struct aiMesh;
struct aiScene;

class MeshImporter {
public:
    MeshImporter();
    ~MeshImporter();

    // IMPORT: Convert from Assimp mesh to our Mesh structure
    static Mesh ImportFromAssimp(const aiMesh* assimpMesh);

    // SAVE: Save our Mesh to custom binary format in Library/Meshes/
    static bool SaveToCustomFormat(const Mesh& mesh, const std::string& filename);

    // LOAD: Load from custom binary format back into our Mesh structure
    static Mesh LoadFromCustomFormat(const std::string& filename);

    // Utility: Generate unique filename for mesh
    static std::string GenerateMeshFilename(const std::string& originalName);
};