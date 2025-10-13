#include "FileSystem.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h> 
#include <assimp/cimport.h>
#include "OpenGL.h"
#include <iostream>
#include "Application.h"

FileSystem::FileSystem() : Module()
{

}

FileSystem:: ~FileSystem() 
{

}

bool FileSystem::Awake()
{
	struct aiLogStream stream;
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_DEBUGGER, nullptr);
	aiAttachLogStream(&stream);
	return true;
}

bool FileSystem::Start()
{
    std::string file_path = "C:\\Users\\aalca\\Documents\\GitHub\\-engine\\Engine\\warrior.FBX";
    LoadFBX(file_path, aiProcessPreset_TargetRealtime_MaxQuality);

    // Loading meshes 
    OpenGL* opengl = Application::GetInstance().opengl.get(); 
    for (auto& mesh : meshes)
    {
        opengl->LoadMesh(mesh);
    }

    std::cout << "Loaded meshes: " << meshes.size() << std::endl;
    for (size_t i = 0; i < meshes.size(); ++i) {
        std::cout << "Mesh " << i << ": "
            << meshes[i].num_vertices << " vertices, "
            << meshes[i].num_indices << " indices" << std::endl;
    }

    return true;
}

bool FileSystem::Update()
{
	return true;
}

void FileSystem::LoadFBX(std::string& file_path, unsigned int flag)
{
    const aiScene* scene = aiImportFile(file_path.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);

    if (scene != nullptr && scene->HasMeshes())
    {
        for (uint i = 0; i < scene->mNumMeshes; ++i)
        {
            aiMesh* aiMesh = scene->mMeshes[i];
            Mesh ourMesh;

            // Copy vertices
            ourMesh.num_vertices = aiMesh->mNumVertices;
            ourMesh.vertices = new float[ourMesh.num_vertices * 3];
            memcpy(ourMesh.vertices, aiMesh->mVertices, sizeof(float) * ourMesh.num_vertices * 3);
           
            for (uint k = 0; k < ourMesh.num_vertices * 3; ++k) {
                ourMesh.vertices[k] *= 0.005f; // reduccion 99.5%
            }
            // Copiar indices
            if (aiMesh->HasFaces())
            {
                ourMesh.num_indices = aiMesh->mNumFaces * 3;
                ourMesh.indices = new uint[ourMesh.num_indices];

                for (uint j = 0; j < aiMesh->mNumFaces; ++j)
                {
                    if (aiMesh->mFaces[j].mNumIndices != 3)
                        std::cout << "WARNING, geometry face with != 3 indices!" << std::endl;
                    else
                        memcpy(&ourMesh.indices[j * 3], aiMesh->mFaces[j].mIndices, 3 * sizeof(uint));
                }
            }

            meshes.push_back(ourMesh);
            std::cout << "to salio perfe" << std::endl;
        }

        aiReleaseImport(scene);
    }
    else
        std::cout << "Error loading scene" << std::endl;
}

bool FileSystem::CleanUp()
{
	aiDetachAllLogStreams();
	return true;
}

