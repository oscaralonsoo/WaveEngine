#include "ModuleAudio.h"

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

ModuleAudio::ModuleAudio()
{
    name = "audio";
}

ModuleAudio::~ModuleAudio()
{
    TerminateWwise();
}

bool ModuleAudio::Awake() { return true; }

bool ModuleAudio::Start()
{
    initialized = InitWwise();
    return initialized;
}

bool ModuleAudio::Update()
{
    if (!initialized) return true;
    AK::SoundEngine::RenderAudio();
    return true;
}

bool ModuleAudio::CleanUp()
{
    TerminateWwise();
    return true;
}

void ModuleAudio::OnPlay()
{
    std::printf("[AUDIO] OnPlay() called. initialized=%d\n", (int)initialized);
    if (!initialized) return;

    // Si estaba pausado -> resume
    if (isPlaying && isPaused)
    {
        std::printf("[AUDIO] Resume\n");
        AK::SoundEngine::ExecuteActionOnEvent(
            AK::EVENTS::MUSICSTARTA,
            AK::SoundEngine::AkActionOnEventType_Resume,
            MUSIC_GO
        );
        isPaused = false;
        return;
    }

    // Si ya está sonando -> no repostear
    if (isPlaying && !isPaused)
    {
        std::printf("[AUDIO] Already playing\n");
        return;
    }

    // Arrancar desde cero
    std::printf("[AUDIO] Play from start\n");
    AK::SoundEngine::StopAll();

    AkPlayingID play = AK::SoundEngine::PostEvent(AK::EVENTS::MUSICSTARTA, MUSIC_GO);
    std::printf("[AUDIO] PostEvent MUSICSTARTA -> %u\n", (unsigned)play);

    if (play == AK_INVALID_PLAYING_ID)
    {
        std::printf("[AUDIO] WARNING: MUSICSTARTA invalid playing id\n");
        return;
    }

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

    isPlaying = false;
    isPaused = false;
}

bool ModuleAudio::InitWwise()
{
    // 1) MemoryMgr
    AkMemSettings memSettings;
    AK::MemoryMgr::GetDefaultSettings(memSettings);

    if (AK::MemoryMgr::Init(&memSettings) != AK_Success)
    {
        std::printf("ERROR: Failed to init MemoryMgr\n");
        return false;
    }
    std::printf("OK: MemoryMgr initialized\n");

    // 2) StreamMgr
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

    // 3) SoundEngine
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

    // 4) GameObject
    result = AK::SoundEngine::RegisterGameObj(MUSIC_GO, "MusicGO");
    if (result != AK_Success)
    {
        std::printf("ERROR: Failed to register GameObject (error: %d)\n", result);
        TerminateWwise();
        return false;
    }
    std::printf("OK: GameObject registered\n");

    // 5) Listener (básico)
    AkGameObjectID listeners[1] = { MUSIC_GO };

    AKRESULT lr = AK::SoundEngine::SetDefaultListeners(listeners, 1);
    if (lr != AK_Success)
    {
        std::printf("ERROR: SetDefaultListeners failed (error: %d)\n", lr);
        TerminateWwise();
        return false;
    }

    lr = AK::SoundEngine::SetListeners(MUSIC_GO, listeners, 1);
    if (lr != AK_Success)
    {
        std::printf("ERROR: SetListeners failed (error: %d)\n", lr);
        TerminateWwise();
        return false;
    }

    std::printf("OK: Listener assigned\n");

    // 6) Banks
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

    std::printf("Wwise initialized successfully!\n");
    return true;
}

void ModuleAudio::TerminateWwise()
{
    if (!initialized) return;

    std::printf("Terminating Wwise...\n");

    AK::SoundEngine::StopAll();
    AK::SoundEngine::RenderAudio();

    AK::SoundEngine::UnregisterGameObj(MUSIC_GO);

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

    std::printf("Wwise terminated successfully\n");
}

bool ModuleAudio::LoadBankFromMemory(const std::filesystem::path& fullPath, AkBankID& outId, std::vector<AkUInt8>& outData)
{
    outId = AK_INVALID_BANK_ID_VALUE;
    outData.clear();

    std::ifstream f(fullPath, std::ios::binary);
    if (!f.is_open()) return false;

    f.seekg(0, std::ios::end);
    const std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);

    if (size <= 0) return false;

    outData.resize((size_t)size);
    if (!f.read((char*)outData.data(), size))
        return false;

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
