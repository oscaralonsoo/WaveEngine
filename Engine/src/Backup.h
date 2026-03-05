#pragma once
#include "Module.h"

class Backup : public Module 
{
public:
	Backup();
	~Backup();
	//bool Awake() override;
	bool Start() override;
	bool Update() override;
	bool CleanUp() override;


private:

	void PerformBackup();

	float timeSinceLastBackup = 0.0f;
	const float backupInterval = 10.0f; // 60 sec (10 rn for testing)
	int backupCounter = 0;
	std::string tempSceneDir;
};

