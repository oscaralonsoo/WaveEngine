#define _HAS_STD_BYTE 0 

#include "MeshImporter.h"
#include <filesystem>
#include "FileSystem.h" 
#include <assimp/mesh.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <limits>

MeshImporter::MeshImporter() {}
MeshImporter::~MeshImporter() {}

// IMPORT: Assimp -> Our Mesh Structure
Mesh MeshImporter::ImportFromAssimp(const aiMesh* assimpMesh) {
    Mesh mesh;

    if (!assimpMesh) {
        LOG_DEBUG("ERROR: assimpMesh is nullptr");
        return mesh;
    }

    LOG_DEBUG("Importing mesh: %s", assimpMesh->mName.C_Str());

    // Reserve space for efficiency
    mesh.vertices.reserve(assimpMesh->mNumVertices);
    mesh.indices.reserve(assimpMesh->mNumFaces * 3);

    // Import vertices
    for (unsigned int i = 0; i < assimpMesh->mNumVertices; i++) {
        Vertex vertex;

        // Position (always present)
        vertex.position = glm::vec3(
            assimpMesh->mVertices[i].x,
            assimpMesh->mVertices[i].y,
            assimpMesh->mVertices[i].z
        );

        // Normals
        if (assimpMesh->HasNormals()) {
            vertex.normal = glm::vec3(
                assimpMesh->mNormals[i].x,
                assimpMesh->mNormals[i].y,
                assimpMesh->mNormals[i].z
            );
        }
        else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        // Texture coordinates
        if (assimpMesh->HasTextureCoords(0)) {
            vertex.texCoords = glm::vec2(
                assimpMesh->mTextureCoords[0][i].x,
                assimpMesh->mTextureCoords[0][i].y
            );
        }
        else {
            vertex.texCoords = glm::vec2(0.0f, 0.0f);
        }

        mesh.vertices.push_back(vertex);
    }

    // Import indices
    for (unsigned int i = 0; i < assimpMesh->mNumFaces; i++) {
        aiFace face = assimpMesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            mesh.indices.push_back(face.mIndices[j]);
        }
    }

    LOG_DEBUG("  Imported: %d vertices, %d indices", mesh.vertices.size(), mesh.indices.size());

    return mesh;
}

// SAVE: Our Mesh -> Custom Binary Format
bool MeshImporter::SaveToCustomFormat(const Mesh& mesh, const std::string& filename) {
    EnsureLibraryDirectoryExists();

    std::string fullPath = "Library/Meshes/" + filename;

    LOG_DEBUG("Saving mesh to: %s", fullPath.c_str());

    std::ofstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        LOG_DEBUG("ERROR: Could not open file for writing: %s", fullPath.c_str());
        return false;
    }

    // 1. Build header
    MeshHeader header;
    header.numVertices = static_cast<unsigned int>(mesh.vertices.size());
    header.numIndices = static_cast<unsigned int>(mesh.indices.size());
    header.hasNormals = !mesh.vertices.empty();
    header.hasTexCoords = !mesh.vertices.empty();

    // Calculate bounding box
    CalculateBoundingBox(mesh, header.boundingBoxMin, header.boundingBoxMax);

    // 2. Write header
    file.write(reinterpret_cast<const char*>(&header), sizeof(MeshHeader));

    // 3. Write vertex data
    if (!mesh.vertices.empty()) {
        file.write(
            reinterpret_cast<const char*>(mesh.vertices.data()),
            mesh.vertices.size() * sizeof(Vertex)
        );
    }

    // 4. Write index data
    if (!mesh.indices.empty()) {
        file.write(
            reinterpret_cast<const char*>(mesh.indices.data()),
            mesh.indices.size() * sizeof(unsigned int)
        );
    }

    file.close();

    LOG_DEBUG("  Mesh saved successfully!");
    LOG_DEBUG("  File size: %zu bytes",
        sizeof(MeshHeader) +
        mesh.vertices.size() * sizeof(Vertex) +
        mesh.indices.size() * sizeof(unsigned int));

    return true;
}

// LOAD: Custom Binary Format -> Our Mesh
Mesh MeshImporter::LoadFromCustomFormat(const std::string& filename) {
    std::string fullPath = "Library/Meshes/" + filename;

    LOG_DEBUG("Loading mesh from: %s", fullPath.c_str());

    Mesh mesh;

    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        LOG_DEBUG("ERROR: Could not open file for reading: %s", fullPath.c_str());
        return mesh;
    }

    // 1. Read header
    MeshHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(MeshHeader));

    LOG_DEBUG("  Header info:");
    LOG_DEBUG("    Vertices: %u", header.numVertices);
    LOG_DEBUG("    Indices: %u", header.numIndices);
    LOG_DEBUG("    Bounding Box: Min(%.2f, %.2f, %.2f) Max(%.2f, %.2f, %.2f)",
        header.boundingBoxMin.x, header.boundingBoxMin.y, header.boundingBoxMin.z,
        header.boundingBoxMax.x, header.boundingBoxMax.y, header.boundingBoxMax.z);

    // 2. Read vertex data
    mesh.vertices.resize(header.numVertices);
    if (header.numVertices > 0) {
        file.read(
            reinterpret_cast<char*>(mesh.vertices.data()),
            header.numVertices * sizeof(Vertex)
        );
    }

    // 3. Read index data
    mesh.indices.resize(header.numIndices);
    if (header.numIndices > 0) {
        file.read(
            reinterpret_cast<char*>(mesh.indices.data()),
            header.numIndices * sizeof(unsigned int)
        );
    }

    file.close();

    LOG_DEBUG("  Mesh loaded successfully!");

    return mesh;
}

// UTILITIES
std::string MeshImporter::GenerateMeshFilename(const std::string& originalName) {
    // Sanitize name: remove spaces and special characters
    std::string sanitized = originalName;
    std::replace(sanitized.begin(), sanitized.end(), ' ', '_');

    // Remove special characters
    sanitized.erase(
        std::remove_if(sanitized.begin(), sanitized.end(),
            [](char c) { return !std::isalnum(c) && c != '_'; }),
        sanitized.end()
    );

    // Add timestamp for uniqueness
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();

    std::stringstream ss;
    ss << sanitized << "_" << timestamp << ".mesh";

    return ss.str();
}

void MeshImporter::CalculateBoundingBox(const Mesh& mesh, glm::vec3& minBounds, glm::vec3& maxBounds) {
    if (mesh.vertices.empty()) {
        minBounds = glm::vec3(0.0f);
        maxBounds = glm::vec3(0.0f);
        return;
    }

    minBounds = glm::vec3(std::numeric_limits<float>::max());
    maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& vertex : mesh.vertices) {
        minBounds.x = std::min(minBounds.x, vertex.position.x);
        minBounds.y = std::min(minBounds.y, vertex.position.y);
        minBounds.z = std::min(minBounds.z, vertex.position.z);

        maxBounds.x = std::max(maxBounds.x, vertex.position.x);
        maxBounds.y = std::max(maxBounds.y, vertex.position.y);
        maxBounds.z = std::max(maxBounds.z, vertex.position.z);
    }
}

void MeshImporter::EnsureLibraryDirectoryExists() {
    std::filesystem::create_directories("Library/Meshes");
}