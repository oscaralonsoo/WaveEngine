#include "ModuleAudio.h"
#include <SDL3/SDL.h>

#define byte win_byte_override
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/AkMemoryMgrModule.h>
#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#undef byte

#include "Wwise_IDs.h"

#include <fstream>
#include <cstdio>
#include <cmath>
#include <filesystem>
#include <vector>
#include <unordered_set>
#include <chrono>

#define AUDIO_DEMO_30_PERCENT 0

#if AUDIO_DEMO_30_PERCENT
static bool  g_staticPosted = false;
static bool  g_dynamicPosted = false;
static float g_t = 0.0f;

static float g_lx = 0.0f;
static float g_ly = 0.0f;
static float g_lz = 0.0f;
#endif

ModuleAudio* ModuleAudio::instance = nullptr;

ModuleAudio::ModuleAudio()
{
    name = "audio";
}

ModuleAudio::~ModuleAudio()
{
    TerminateWwise();
}

bool ModuleAudio::Awake()
{
    return true;
}

bool ModuleAudio::Start()
{
    instance = this;
    initialized = InitWwise();
    return initialized;
}

bool ModuleAudio::Update()
{
    if (!initialized) return true;

#if AUDIO_DEMO_30_PERCENT
    const bool* keys = SDL_GetKeyboardState(nullptr);
    const float speed = 0.10f;

    if (keys[SDL_SCANCODE_W]) g_lz += speed;
    if (keys[SDL_SCANCODE_S]) g_lz -= speed;
    if (keys[SDL_SCANCODE_A]) g_lx -= speed;
    if (keys[SDL_SCANCODE_D]) g_lx += speed;

    SetGameObjectTransform(
        DEFAULT_LISTENER_GO,
        g_lx, g_ly, g_lz,
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f
    );

    SetGameObjectTransform(
        STATIC_SFX_GO,
        5.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f
    );

    g_t += 0.02f;
    const float x = std::sinf(g_t) * 6.0f;
    const float z = 4.0f + std::cosf(g_t) * 2.0f;

    SetGameObjectTransform(
        DYNAMIC_SFX_GO,
        x, 0.0f, z,
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f
    );

    if (!g_staticPosted)
    {
        AkPlayingID pid = AK::SoundEngine::PostEvent(AK::EVENTS::SFX_STATIC_PLAY, STATIC_SFX_GO);
        std::printf("[AUDIO] PostEvent SFX_STATIC_PLAY -> %u\n", (unsigned)pid);
        g_staticPosted = true;
    }

    if (!g_dynamicPosted)
    {
        AkPlayingID pid = AK::SoundEngine::PostEvent(AK::EVENTS::SFX_DYNAMIC_PLAY, DYNAMIC_SFX_GO);
        std::printf("[AUDIO] PostEvent SFX_DYNAMIC_PLAY -> %u\n", (unsigned)pid);
        g_dynamicPosted = true;
    }
#endif

    AK::SoundEngine::RenderAudio();
    return true;
}

bool ModuleAudio::CleanUp()
{
    TerminateWwise();
    if (instance == this) instance = nullptr;
    return true;
}

void ModuleAudio::OnPlay()
{
    if (!initialized) return;

    if (isPlaying && isPaused)
    {
        AK::SoundEngine::ExecuteActionOnEvent(
            AK::EVENTS::MUSICSTARTA,
            AK::SoundEngine::AkActionOnEventType_Resume,
            MUSIC_GO
        );
        isPaused = false;
        return;
    }

    if (isPlaying && !isPaused) return;

    AK::SoundEngine::ExecuteActionOnEvent(
        AK::EVENTS::MUSICSTARTA,
        AK::SoundEngine::AkActionOnEventType_Stop,
        MUSIC_GO
    );

    AkPlayingID play = AK::SoundEngine::PostEvent(AK::EVENTS::MUSICSTARTA, MUSIC_GO);

    if (play == AK_INVALID_PLAYING_ID)
        return;

    isPlaying = true;
    isPaused = false;
}

void ModuleAudio::OnPause()
{
    if (!initialized) return;
    if (!isPlaying) return;
    if (isPaused) return;

    std::printf("[AUDIO] Pause\n");
    AK::SoundEngine::ExecuteActionOnEvent(
        AK::EVENTS::MUSICSTARTA,
        AK::SoundEngine::AkActionOnEventType_Pause,
        MUSIC_GO
    );
    isPaused = true;
}

void ModuleAudio::OnStop()
{
    if (!initialized) return;

    std::printf("[AUDIO] Stop\n");

    AK::SoundEngine::ExecuteActionOnEvent(
        AK::EVENTS::MUSICSTARTA,
        AK::SoundEngine::AkActionOnEventType_Stop,
        MUSIC_GO
    );

    AK::SoundEngine::StopAll();

#if AUDIO_DEMO_30_PERCENT
    g_staticPosted = false;
    g_dynamicPosted = false;
#endif

    isPlaying = false;
    isPaused = false;
}

void ModuleAudio::SetRTPC(unsigned int rtpcId, float value, AkGameObjectID gameObjectId)
{
    if (!initialized) return;

    AkGameObjectID target = (gameObjectId == 0) ? AK_INVALID_GAME_OBJECT : gameObjectId;

    AKRESULT r = AK::SoundEngine::SetRTPCValue((AkRtpcID)rtpcId, value, target);

    // cache debug (lo último que seteas tú)
    rtpcCacheById[rtpcId] = value;

    if (r != AK_Success)
    {
        std::printf("[AUDIO] SetRTPCValue failed rtpc=%u value=%.2f go=%llu err=%d\n",
            rtpcId, value, (unsigned long long)target, (int)r);
    }
}

void ModuleAudio::SetRTPCByName(const char* rtpcName, float value, AkGameObjectID gameObjectId)
{
    if (!initialized || !rtpcName || !rtpcName[0]) return;

    unsigned int id = 0;
    auto it = rtpcNameToIdCache.find(rtpcName);
    if (it != rtpcNameToIdCache.end())
    {
        id = it->second;
    }
    else
    {
        // Esto NO es Query API, es hashing interno (sí está disponible)
        id = (unsigned int)AK::SoundEngine::GetIDFromString(rtpcName);
        rtpcNameToIdCache[rtpcName] = id;
    }

    SetRTPC(id, value, gameObjectId);
}

float ModuleAudio::GetRTPCCached(unsigned int rtpcId) const
{
    auto it = rtpcCacheById.find(rtpcId);
    if (it == rtpcCacheById.end()) return 0.0f;
    return it->second;
}

float ModuleAudio::GetRTPCCachedByName(const char* rtpcName) const
{
    if (!rtpcName || !rtpcName[0]) return 0.0f;

    unsigned int id = 0;
    auto it = rtpcNameToIdCache.find(rtpcName);
    if (it != rtpcNameToIdCache.end())
        id = it->second;
    else
        id = (unsigned int)AK::SoundEngine::GetIDFromString(rtpcName);

    return GetRTPCCached(id);
}

bool ModuleAudio::RegisterAudioGameObject(AkGameObjectID id, const char* debugName)
{
    if (!initialized) return false;

    if (registeredGOs.find(id) != registeredGOs.end())
        return true;

    AKRESULT r = AK::SoundEngine::RegisterGameObj(id, debugName);
    if (r != AK_Success)
    {
        std::printf("[AUDIO] RegisterGameObj failed id=%llu err=%d\n",
            (unsigned long long)id, (int)r);
        return false;
    }

    AKRESULT lr = AK::SoundEngine::SetListeners(id, &currentListener, 1);
    if (lr != AK_Success)
    {
        std::printf("[AUDIO] SetListeners failed emitter=%llu listener=%llu err=%d\n",
            (unsigned long long)id, (unsigned long long)currentListener, (int)lr);
    }

    registeredGOs.insert(id);
    return true;
}

void ModuleAudio::UnregisterAudioGameObject(AkGameObjectID id)
{
    if (!initialized) return;

    if (registeredGOs.find(id) == registeredGOs.end())
        return;

    AK::SoundEngine::UnregisterGameObj(id);
    registeredGOs.erase(id);
}

void ModuleAudio::SetListener(AkGameObjectID listenerId)
{
    if (!initialized) return;

    currentListener = listenerId;

    AKRESULT dl = AK::SoundEngine::SetDefaultListeners(&currentListener, 1);
    if (dl != AK_Success)
        std::printf("[AUDIO] SetDefaultListeners failed err=%d\n", (int)dl);

    for (AkGameObjectID go : registeredGOs)
    {
        AKRESULT lr = AK::SoundEngine::SetListeners(go, &currentListener, 1);
        if (lr != AK_Success)
        {
            std::printf("[AUDIO] SetListeners failed emitter=%llu listener=%llu err=%d\n",
                (unsigned long long)go, (unsigned long long)currentListener, (int)lr);
        }
    }
}

void ModuleAudio::SetGameObjectTransform(
    AkGameObjectID id,
    float px, float py, float pz,
    float fx, float fy, float fz,
    float ux, float uy, float uz)
{
    if (!initialized) return;

    AkSoundPosition pos;
    pos.SetPosition(px, py, pz);
    pos.SetOrientation(fx, fy, fz, ux, uy, uz);

    AK::SoundEngine::SetPosition(id, pos);
}

unsigned int ModuleAudio::PostEvent(unsigned int eventId, AkGameObjectID gameObjectId)
{
    if (!initialized) return 0;

    AkPlayingID pid = AK::SoundEngine::PostEvent((AkUniqueID)eventId, gameObjectId);
    if (pid == AK_INVALID_PLAYING_ID)
    {
        std::printf("[AUDIO] PostEvent failed event=%u go=%llu\n",
            eventId, (unsigned long long)gameObjectId);
        return 0;
    }
    return (unsigned int)pid;
}

bool ModuleAudio::InitWwise()
{
    AkMemSettings memSettings;
    AK::MemoryMgr::GetDefaultSettings(memSettings);

    if (AK::MemoryMgr::Init(&memSettings) != AK_Success)
    {
        std::printf("ERROR: Failed to init MemoryMgr\n");
        return false;
    }
    std::printf("OK: MemoryMgr initialized\n");

    AkStreamMgrSettings streamSettings;
    AK::StreamMgr::GetDefaultSettings(streamSettings);

    streamMgr = AK::StreamMgr::Create(streamSettings);
    if (!streamMgr)
    {
        std::printf("ERROR: Failed to create StreamMgr\n");
        AK::MemoryMgr::Term();
        return false;
    }
    std::printf("OK: StreamMgr created\n");

    AkInitSettings initSettings;
    AkPlatformInitSettings platformSettings;
    AK::SoundEngine::GetDefaultInitSettings(initSettings);
    AK::SoundEngine::GetDefaultPlatformInitSettings(platformSettings);

    initSettings.pfnAssertHook = nullptr;

    AKRESULT result = AK::SoundEngine::Init(&initSettings, &platformSettings);
    if (result != AK_Success)
    {
        std::printf("ERROR: Failed to init SoundEngine (error code: %d)\n", result);
        streamMgr->Destroy();
        streamMgr = nullptr;
        AK::MemoryMgr::Term();
        return false;
    }
    std::printf("OK: SoundEngine initialized\n");

    result = AK::SoundEngine::RegisterGameObj(MUSIC_GO, "MusicGO");
    if (result != AK_Success)
    {
        std::printf("ERROR: Failed to register MusicGO (error: %d)\n", result);
        TerminateWwise();
        return false;
    }

    result = AK::SoundEngine::RegisterGameObj(DEFAULT_LISTENER_GO, "DefaultListenerGO");
    if (result != AK_Success)
    {
        std::printf("ERROR: Failed to register DefaultListenerGO (error: %d)\n", result);
        TerminateWwise();
        return false;
    }

    registeredGOs.insert(MUSIC_GO);
    registeredGOs.insert(DEFAULT_LISTENER_GO);

    currentListener = DEFAULT_LISTENER_GO;

    AKRESULT lr = AK::SoundEngine::SetDefaultListeners(&currentListener, 1);
    if (lr != AK_Success)
    {
        std::printf("ERROR: SetDefaultListeners failed (error: %d)\n", lr);
        TerminateWwise();
        return false;
    }

    lr = AK::SoundEngine::SetListeners(MUSIC_GO, &currentListener, 1);
    if (lr != AK_Success)
    {
        std::printf("ERROR: SetListeners(MUSIC_GO) failed (error: %d)\n", lr);
        TerminateWwise();
        return false;
    }

#if AUDIO_DEMO_30_PERCENT
    RegisterAudioGameObject(STATIC_SFX_GO, "StaticSFX_GO");
    RegisterAudioGameObject(DYNAMIC_SFX_GO, "DynamicSFX_GO");
#endif

    std::printf("OK: Listener assigned (DefaultListenerGO)\n");

    const std::filesystem::path bankDir =
        R"(C:\Users\Arnau\OneDrive\Documentos\GitHub\Motor2025\Audio\WwiseProject\MusicEngine\GeneratedSoundBanks\Windows)";

    if (!std::filesystem::exists(bankDir))
    {
        std::printf("ERROR: Bank directory does not exist\n");
        TerminateWwise();
        return false;
    }
    std::printf("OK: Bank directory exists\n");

    if (!LoadBankFromMemory(bankDir / "Init.bnk", initBankId, initBankData))
    {
        std::printf("ERROR: Failed to load Init.bnk\n");
        TerminateWwise();
        return false;
    }
    std::printf("OK: Init.bnk loaded (ID: %u)\n", initBankId);

    if (!LoadBankFromMemory(bankDir / "Main.bnk", mainBankId, mainBankData))
    {
        std::printf("ERROR: Failed to load Main.bnk\n");
        TerminateWwise();
        return false;
    }
    std::printf("OK: Main.bnk loaded (ID: %u)\n", mainBankId);

    // Check ID TunnelAmount (hash vs header)
    unsigned int byName = (unsigned int)AK::SoundEngine::GetIDFromString("TunnelAmount");
    unsigned int byHeader = (unsigned int)AK::GAME_PARAMETERS::TUNNELAMOUNT;
    std::printf("[AUDIO] TunnelAmount ID check: byName=%u byHeader=%u\n", byName, byHeader);

    std::printf("Wwise initialized successfully!\n");
    return true;
}

void ModuleAudio::TerminateWwise()
{
    if (!initialized) return;

    std::printf("Terminating Wwise...\n");

    AK::SoundEngine::StopAll();
    AK::SoundEngine::RenderAudio();

    for (AkGameObjectID go : registeredGOs)
        AK::SoundEngine::UnregisterGameObj(go);

    registeredGOs.clear();

    if (mainBankId != AK_INVALID_BANK_ID_VALUE)
    {
        AK::SoundEngine::UnloadBank(mainBankId, nullptr);
        mainBankData.clear();
        mainBankId = AK_INVALID_BANK_ID_VALUE;
    }

    if (initBankId != AK_INVALID_BANK_ID_VALUE)
    {
        AK::SoundEngine::UnloadBank(initBankId, nullptr);
        initBankData.clear();
        initBankId = AK_INVALID_BANK_ID_VALUE;
    }

    AK::SoundEngine::Term();

    if (streamMgr)
    {
        streamMgr->Destroy();
        streamMgr = nullptr;
    }

    AK::MemoryMgr::Term();

    initialized = false;
    isPlaying = false;
    isPaused = false;

    if (instance == this) instance = nullptr;

    std::printf("Wwise terminated successfully\n");
}

bool ModuleAudio::LoadBankFromMemory(
    const std::filesystem::path& fullPath,
    AkBankID& outId,
    std::vector<AkUInt8>& outData)
{
    outId = AK_INVALID_BANK_ID_VALUE;
    outData.clear();

    if (!std::filesystem::exists(fullPath))
        return false;

    // debug: tamaño + timestamp del archivo
    const auto fsize = std::filesystem::file_size(fullPath);
    const auto ftime = std::filesystem::last_write_time(fullPath).time_since_epoch().count();
    std::printf("[AUDIO] Bank file: %ls size=%llu lastWrite=%lld\n",
        fullPath.wstring().c_str(),
        (unsigned long long)fsize,
        (long long)ftime);

    std::ifstream f(fullPath, std::ios::binary);
    if (!f.is_open()) return false;

    f.seekg(0, std::ios::end);
    const std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);
    if (size <= 0) return false;

    outData.resize((size_t)size);
    if (!f.read((char*)outData.data(), size)) return false;

    AkBankID tmpId = AK_INVALID_BANK_ID_VALUE;
    const AKRESULT res = AK::SoundEngine::LoadBankMemoryView(
        outData.data(),
        (AkUInt32)outData.size(),
        tmpId
    );

    if (res != AK_Success) return false;

    outId = tmpId;
    return true;
}

void ModuleAudio::UnloadBankFromMemory(AkBankID id, std::vector<AkUInt8>& data)
{
    if (id != AK_INVALID_BANK_ID_VALUE)
    {
        AK::SoundEngine::UnloadBank(id, nullptr);
        data.clear();
    }
}
