#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
struct Mesh;
struct aiMesh;
struct aiScene;

// Header structure for custom mesh format
struct MeshHeader {
    unsigned int numVertices = 0;
    unsigned int numIndices = 0;
    bool hasNormals = false;
    bool hasTexCoords = false;

    // Pre-calculated data
    glm::vec3 boundingBoxMin = glm::vec3(0.0f);
    glm::vec3 boundingBoxMax = glm::vec3(0.0f);
};

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

private:
    // Calculate bounding box from mesh vertices
    static void CalculateBoundingBox(const Mesh& mesh, glm::vec3& minBounds, glm::vec3& maxBounds);
};