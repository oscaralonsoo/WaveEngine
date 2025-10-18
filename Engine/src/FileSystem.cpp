#include "FileSystem.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h> 
#include <assimp/cimport.h>
#include <iostream>
#include "Application.h"

FileSystem::FileSystem() : Module() {}
FileSystem::~FileSystem() {}

bool FileSystem::Awake()
{
	return true;
}

bool FileSystem::Start()
{
	return true;
}

bool FileSystem::Update()
{
	if (Application::GetInstance().input->HasDroppedFile())
	{
		std::string filePath = Application::GetInstance().input->GetDroppedFilePath();
		Application::GetInstance().input->ClearDroppedFile();

		// Verificar si es un archivo FBX
		if (filePath.size() >= 4)
		{
			std::string extension = filePath.substr(filePath.size() - 4);
			// Convertir a minúsculas para comparar
			for (char& c : extension)
				c = tolower(c);

			if (extension == ".fbx")
			{
				// Limpiar meshes anteriores antes de cargar nuevos
				ClearMeshes();

				// Cargar el nuevo archivo
				if (LoadFBX(filePath))
				{
					std::cout << "Successfully loaded FBX file!" << std::endl;
				}
				else
				{
					std::cerr << "Failed to load FBX file!" << std::endl;
				}
			}
			else
			{
				std::cerr << "Invalid file format. Please drop an FBX file." << std::endl;
			}
		}
	}

	return true;
}

bool FileSystem::LoadFBX(const std::string& file_path, unsigned int flag)
{
	std::cout << "Loading FBX: " << file_path << std::endl;

	const aiScene* scene = aiImportFile(file_path.c_str(),
		flag != 0 ? flag : aiProcessPreset_TargetRealtime_MaxQuality);

	if (scene == nullptr)
	{
		std::cerr << "Error loading: " << aiGetErrorString() << std::endl;
		return false;
	}

	if (!scene->HasMeshes())
	{
		std::cerr << "No meshes in scene" << std::endl;
		aiReleaseImport(scene);
		return false;
	}

	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		aiMesh* aiMesh = scene->mMeshes[i];
		Mesh mesh;

		mesh.num_vertices = aiMesh->mNumVertices;
		mesh.vertices = new float[mesh.num_vertices * 3];
		memcpy(mesh.vertices, aiMesh->mVertices, sizeof(float) * mesh.num_vertices * 3);

		// Indices
		if (aiMesh->HasFaces())
		{
			mesh.num_indices = aiMesh->mNumFaces * 3;
			mesh.indices = new unsigned int[mesh.num_indices];

			for (unsigned int j = 0; j < aiMesh->mNumFaces; ++j)
				memcpy(&mesh.indices[j * 3], aiMesh->mFaces[j].mIndices, 3 * sizeof(unsigned int));
		}

		Application::GetInstance().renderer->LoadMesh(mesh);
		meshes.push_back(mesh);

		std::cout << "Mesh " << i << ": " << mesh.num_vertices << " vertices, " << mesh.num_indices << " indices" << std::endl;
	}

	aiReleaseImport(scene);
	return true;
}

bool FileSystem::LoadFBXFromAssets(const std::string& filename)
{
	return LoadFBX("Assets/" + filename, aiProcessPreset_TargetRealtime_MaxQuality);
}

void FileSystem::AddMesh(const Mesh& mesh)
{
	meshes.push_back(mesh);
}

void FileSystem::ClearMeshes()
{
	for (auto& mesh : meshes)
	{
		Application::GetInstance().renderer->UnloadMesh(mesh);
		delete[] mesh.vertices;
		delete[] mesh.indices;
	}
	meshes.clear();
	std::cout << "All meshes cleared" << std::endl;
}

bool FileSystem::CleanUp()
{
	ClearMeshes();
	aiDetachAllLogStreams();
	return true;
}