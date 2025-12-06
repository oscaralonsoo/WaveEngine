#include "ResourceMesh.h"
#include "MeshImporter.h"
#include "Log.h"
#include <glad/glad.h>

ResourceMesh::ResourceMesh(UID uid)
    : Resource(uid, Resource::MESH) {
}

ResourceMesh::~ResourceMesh() {
    UnloadFromMemory();
}

bool ResourceMesh::LoadInMemory() {
    if (loadedInMemory) {
        LOG_DEBUG("[ResourceMesh] Already loaded in memory");
        return true;
    }

    if (libraryFile.empty()) {
        LOG_DEBUG("[ResourceMesh] ERROR: No library file specified");
        return false;
    }

    LOG_DEBUG("[ResourceMesh] Loading mesh from: %s", libraryFile.c_str());

    // only extract the name of the file
    std::string filename = libraryFile;
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }

    // load using meshimporter
    mesh = MeshImporter::LoadFromCustomFormat(filename);

    if (mesh.vertices.empty() || mesh.indices.empty()) {
        LOG_DEBUG("[ResourceMesh] ERROR: Failed to load mesh data");
        return false;
    }

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER,
        mesh.vertices.size() * sizeof(Vertex),
        mesh.vertices.data(),
        GL_STATIC_DRAW);

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        mesh.indices.size() * sizeof(unsigned int),
        mesh.indices.data(),
        GL_STATIC_DRAW);

    // Atributos de vértice
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, normal));

    // TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        (void*)offsetof(Vertex, texCoords));

    glBindVertexArray(0);

    loadedInMemory = true;

    LOG_DEBUG("[ResourceMesh] Loaded successfully:");
    LOG_DEBUG("  VAO: %u", mesh.VAO);
    LOG_DEBUG("  Vertices: %zu", mesh.vertices.size());
    LOG_DEBUG("  Triangles: %zu", mesh.indices.size() / 3);

    return true;
}

void ResourceMesh::UnloadFromMemory() {
    if (!loadedInMemory) {
        return;
    }

    LOG_DEBUG("[ResourceMesh] Unloading mesh VAO: %u", mesh.VAO);

    if (mesh.VAO != 0) {
        glDeleteVertexArrays(1, &mesh.VAO);
        mesh.VAO = 0;
    }

    if (mesh.VBO != 0) {
        glDeleteBuffers(1, &mesh.VBO);
        mesh.VBO = 0;
    }

    if (mesh.EBO != 0) {
        glDeleteBuffers(1, &mesh.EBO);
        mesh.EBO = 0;
    }

    mesh.vertices.clear();
    mesh.indices.clear();
    mesh.textures.clear();

    loadedInMemory = false;
}