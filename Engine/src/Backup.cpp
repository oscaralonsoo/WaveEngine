#include "Backup.h"
#include "Log.h"
#include "Application.h"
#include "Time.h"
#include <filesystem>
Backup::Backup()
{
}

Backup::~Backup()
{
}

bool Backup::Start()
{
	std::filesystem::path tempScenePath = std::filesystem::current_path().parent_path()/"TempScene";

	if (!std::filesystem::exists(tempScenePath)) 
	{
		std::filesystem::create_directory(tempScenePath);
		LOG_CONSOLE("[BACKUP]TempScene directory created.");
	}
	else 
	{
		LOG_CONSOLE("[BACKUP]TempScene directory already exists.");
	}

	tempSceneDir = tempScenePath.string();
	timeSinceLastBackup = 0.0f;
	backupCounter = 0;

	return true;
}

bool Backup::Update()
{
	timeSinceLastBackup += Application::GetInstance().time->GetRealDeltaTime();

	if (timeSinceLastBackup >= backupInterval)
	{
		PerformBackup();
		timeSinceLastBackup = 0.0f;
	}

	return true;
}

bool Backup::CleanUp()
{
	return true;
}

void Backup::PerformBackup() 
{
	backupCounter++;
	std::string backupFilename = tempSceneDir + "/backup_" + std::to_string(backupCounter) + ".json";

	bool success = Application::GetInstance().scene->SaveScene(backupFilename);

	if (success)
	{
		LOG_CONSOLE("[BACKUP] Scene saved: %s", backupFilename.c_str());
	}
	else
	{
		LOG_CONSOLE("[BACKUP] ERROR: Failed to save scene: %s", backupFilename.c_str());
	}
}