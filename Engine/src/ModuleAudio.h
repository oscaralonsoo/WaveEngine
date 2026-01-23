#pragma once

#include "Module.h"
#include <vector>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include <string>


#include "Wwise_IDs.h"

namespace AK { class IAkStreamMgr; }


using AkBankID = unsigned int;
using AkGameObjectID = unsigned long long;
using AkRtpcID = unsigned int;
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

    static ModuleAudio* Get() { return instance; }

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

    // RTPC
    void SetRTPC(unsigned int rtpcId, float value, AkGameObjectID gameObjectId = 0);
    void SetRTPCByName(const char* rtpcName, float value, AkGameObjectID gameObjectId = 0);

    float GetRTPCCached(unsigned int rtpcId) const;
    float GetRTPCCachedByName(const char* rtpcName) const;

    // SFX Volume 
    void SetSfxVolume(float volume01);
    float GetSfxVolume() const { return sfxVolume01; }
    AkGameObjectID GetMusicGameObjectId() const { return MUSIC_GO; }
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

    static constexpr AkGameObjectID MUSIC_GO = 1;
    static constexpr AkGameObjectID DEFAULT_LISTENER_GO = 100;
    static constexpr AkGameObjectID STATIC_SFX_GO = 200;
    static constexpr AkGameObjectID DYNAMIC_SFX_GO = 201;

    AkGameObjectID currentListener = DEFAULT_LISTENER_GO;

    std::unordered_set<AkGameObjectID> registeredGOs;

    bool isPlaying = false;
    bool isPaused = false;

   
    std::unordered_map<unsigned int, float> rtpcCacheById;
    mutable std::unordered_map<std::string, unsigned int> rtpcNameToIdCache;

    
    float sfxVolume01 = 1.0f;
};
