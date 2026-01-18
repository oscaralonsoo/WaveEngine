#pragma once

#include "Module.h"
#include <vector>
#include <filesystem>

// Forward declaration (sin headers Wwise aquí)
namespace AK { class IAkStreamMgr; }

// Tipos mínimos (para no depender de includes Wwise en el header)
using AkBankID = unsigned int;
using AkGameObjectID = unsigned long long;
using AkUInt8 = unsigned char;

static constexpr AkBankID AK_INVALID_BANK_ID_VALUE = 0xFFFFFFFFu;

class ModuleAudio : public Module
{
public:
    ModuleAudio();
    ~ModuleAudio();

    bool Awake() override;
    bool Start() override;
    bool Update() override;
    bool CleanUp() override;

    void OnPlay();
    void OnPause();
    void OnStop();

private:
    bool InitWwise();
    void TerminateWwise();

    bool LoadBankFromMemory(const std::filesystem::path& fullPath, AkBankID& outId, std::vector<AkUInt8>& outData);
    void UnloadBankFromMemory(AkBankID id, std::vector<AkUInt8>& data);

private:
    bool initialized = false;

    AkBankID initBankId = AK_INVALID_BANK_ID_VALUE;
    AkBankID mainBankId = AK_INVALID_BANK_ID_VALUE;

    std::vector<AkUInt8> initBankData;
    std::vector<AkUInt8> mainBankData;

    AK::IAkStreamMgr* streamMgr = nullptr;

    static constexpr AkGameObjectID MUSIC_GO = 1;

    bool isPlaying = false;
    bool isPaused = false;
};
