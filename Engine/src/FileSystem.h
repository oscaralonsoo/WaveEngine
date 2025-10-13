#pragma once

#include "Module.h"
#include <iostream>
#include <vector> 

struct Mesh {
	unsigned int num_vertices = 0;
	float* vertices = nullptr;

	unsigned int num_indices = 0;
	unsigned int* indices = nullptr;

	unsigned int id_vertex = 0;   // VBO
	unsigned int id_index = 0;    // EBO
	unsigned int id_VAO = 0;      
};

class FileSystem : public Module
{
public:
	FileSystem();
	~FileSystem();

	bool Awake() override;

	// Called before the first frame
	bool Start() override;

	// Called each loop iteration
	bool Update() override;

	// Called before quitting
	bool CleanUp() override;

	std::vector<Mesh> meshes;

	const std::vector<Mesh>& GetMeshes() const { return meshes; }

private:
	void LoadFBX(std::string& file_path, unsigned int flag);

	uint id_index = 0; // index in VRAM
	uint num_index = 0;
	uint* index = nullptr;
	uint id_vertex = 0; // unique vertex in VRAM
	uint num_vertex = 0;
	float* vertex = nullptr;
};

