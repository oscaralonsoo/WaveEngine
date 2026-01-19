#pragma once

#include "Module.h"
#include <vector>
#include <filesystem>
#include <unordered_set>

// Forward declaration (sin headers Wwise aquí)
namespace AK { class IAkStreamMgr; }

// Tipos mínimos para no depender de includes Wwise en el header
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

    // Tu control de música (no tocar flujo)
    void OnPlay();
    void OnPause();
    void OnStop();

    // --------- NUEVO: acceso global para componentes (sin App) ----------
    static ModuleAudio* Get() { return instance; }

    // --------- NUEVO: API para GameObjects con audio 3D ----------
    bool RegisterAudioGameObject(AkGameObjectID id, const char* debugName);
    void UnregisterAudioGameObject(AkGameObjectID id);

    void SetGameObjectTransform(
        AkGameObjectID id,
        float px, float py, float pz,
        float fx, float fy, float fz,
        float ux, float uy, float uz
    );

    void SetListener(AkGameObjectID listenerId);

    unsigned int PostEvent(unsigned int eventId, AkGameObjectID gameObjectId);

private:
    bool InitWwise();
    void TerminateWwise();

    bool LoadBankFromMemory(const std::filesystem::path& fullPath, AkBankID& outId, std::vector<AkUInt8>& outData);
    void UnloadBankFromMemory(AkBankID id, std::vector<AkUInt8>& data);

private:
    static ModuleAudio* instance;

    bool initialized = false;

    AkBankID initBankId = AK_INVALID_BANK_ID_VALUE;
    AkBankID mainBankId = AK_INVALID_BANK_ID_VALUE;

    std::vector<AkUInt8> initBankData;
    std::vector<AkUInt8> mainBankData;

    AK::IAkStreamMgr* streamMgr = nullptr;

    // GO de música (tu sistema)
    static constexpr AkGameObjectID MUSIC_GO = 1;

    // GO listener por defecto para 3D
  

    static constexpr AkGameObjectID DEFAULT_LISTENER_GO = 100;
    static constexpr AkGameObjectID STATIC_SFX_GO = 200;
    static constexpr AkGameObjectID DYNAMIC_SFX_GO = 201;


    // Listener actual
    AkGameObjectID currentListener = DEFAULT_LISTENER_GO;

    // Para saber qué GOs hemos registrado
    std::unordered_set<AkGameObjectID> registeredGOs;

    bool isPlaying = false;
    bool isPaused = false;
};
