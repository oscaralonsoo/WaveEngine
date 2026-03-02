#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "Globals.h"

std::string GetDirectoryFromPath(const std::string& filePath);

std::string GetFileExtension(const std::string& filePath);

std::string GetFileName(const std::string& filePath);

std::string FindFileInDirectory(const std::string& directoryPath, const std::string& fileName);

std::string GetPreviousPath(const std::string& directoryPath);

std::vector<std::string> GetListDirectoryContents(const std::string& directoryPath, bool recursive = false);

bool IsFileDirectory(const std::string& directoryPath);

bool DoesFileExist(const std::string& filePath);

bool DoesFileHasMeta(const std::string& directoryPath);

std::string GetMetaPath(const std::string& directoryPath);

std::string GetLibraryPath(const UID uid);

bool CreateDirectory(const std::string& directoryPath);

int64_t GetLastModificationTime(const std::string& path);

uint32_t GetFileHash(const std::string& path);

bool MoveAssetToFolder(const std::string& oldPath, const std::string& destinationFolder);

bool CopyAssetToFolder(const std::string& oldPath, const std::string& destinationFolder);

bool DeletePath(const std::string& path);

bool DeleteAsset(const std::string& path);

std::string GetFileNameNoExtension(const std::string& path);

std::string GetCleanPath(const std::string& path);