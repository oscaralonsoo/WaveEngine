#define _HAS_STD_BYTE 0 

#include "MeshImporter.h"
#include "LibraryManager.h"
#include <filesystem>
#include "ModuleLoader.h" 
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

    return mesh;
}

// SAVE: Our Mesh -> Custom Binary Format
bool MeshImporter::SaveToCustomFormat(const Mesh& mesh, const std::string& filename) {
    std::string fullPath = LibraryManager::GetMeshPath(filename);

    std::ofstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        LOG_DEBUG("ERROR: Could not open file for writing: %s", fullPath.c_str());
        return false;
    }

    unsigned int numVertices = static_cast<unsigned int>(mesh.vertices.size());
    unsigned int numIndices = static_cast<unsigned int>(mesh.indices.size());

    file.write(reinterpret_cast<const char*>(&numVertices), sizeof(unsigned int));
    file.write(reinterpret_cast<const char*>(&numIndices), sizeof(unsigned int));

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

    return true;
}

Mesh MeshImporter::LoadFromCustomFormat(const std::string& filename) {

    std::string fullPath = LibraryManager::GetMeshPath(filename);
    Mesh mesh;

    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        LOG_DEBUG("ERROR: Could not open file for reading: %s", fullPath.c_str());
        return mesh;
    }

    unsigned int numVertices = 0;
    unsigned int numIndices = 0;

    file.read(reinterpret_cast<char*>(&numVertices), sizeof(unsigned int));
    file.read(reinterpret_cast<char*>(&numIndices), sizeof(unsigned int));

    if (numVertices > 0) {
        mesh.vertices.resize(numVertices);
        file.read(
            reinterpret_cast<char*>(mesh.vertices.data()),
            numVertices * sizeof(Vertex)
        );
    }

    if (numIndices > 0) {
        mesh.indices.resize(numIndices);
        file.read(
            reinterpret_cast<char*>(mesh.indices.data()),
            numIndices * sizeof(unsigned int)
        );
    }

    file.close();

    LOG_DEBUG("Mesh loaded from Library: %s (%u verts, %u indices)",
        filename.c_str(), numVertices, numIndices);

    return mesh;
}

std::string MeshImporter::GenerateMeshFilename(const std::string& originalName) {
    std::string sanitized = originalName;
    if (sanitized.empty()) {
        sanitized = "unnamed_mesh";
    }

    std::replace(sanitized.begin(), sanitized.end(), ' ', '_');

    sanitized.erase(
        std::remove_if(sanitized.begin(), sanitized.end(),
            [](char c) { return !std::isalnum(c) && c != '_'; }),
        sanitized.end()
    );

    std::transform(sanitized.begin(), sanitized.end(), sanitized.begin(), ::tolower);

    std::hash<std::string> hasher;
    size_t hashValue = hasher(originalName);

    std::stringstream ss;
    ss << sanitized << "_" << std::hex << hashValue << ".mesh";

    return ss.str();
}