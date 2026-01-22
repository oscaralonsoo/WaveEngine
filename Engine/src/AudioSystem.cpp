#pragma comment(lib, "ws2_32.lib")

#ifndef _UCRT_NO_CPP_LIBC_MACROS
#define _UCRT_NO_CPP_LIBC_MACROS
#endif

#include <ctime>
#include <iostream>
#include <cwchar>
#include <fstream>
#include <nlohmann/json.hpp>
#include "AudioSystem.h"
#include "AudioUtility.h"
#include "AudioComponent.h"
#include "Log.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SpatialAudio/Common/AkSpatialAudio.h>

#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h> 
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkGeneratedSoundBanksResolver.h"

#include <windows.h>

AudioEvent::AudioEvent() {
    playingID = 0L;
    eventCallback = (AkCallbackFunc)AudioSystem::EventCallBack;
}

AudioSystem::AudioSystem() {

}

AudioSystem::~AudioSystem() {

}

bool AudioSystem::InitEngine() {

    LOG_DEBUG("# Initializing Audio Engine...");

    //Wwise submodules must be initialized in the following order:
    if (!InitMemoryManager()) {
        LOG_CONSOLE("Failed to initialize Wwise's Memory Manager");
        return false;
    }

    if (!InitStreamingManager()) {
        LOG_CONSOLE("Failed to initialize Wwise's Streaming Manager");
        return false;
    }

    if (!InitSoundEngine()) {
        LOG_CONSOLE("Failed to initialize Wwise's Sound Engine");
        return false;
    }
    
    if (!InitSpatialAudio()) {
        LOG_CONSOLE("Failed to initialize Wwise's Spatial Audio");
        return false;
    }

#ifndef AK_OPTIMIZED
    if (!InitCommunication()) {
        LOG_CONSOLE("Failed to initialize Wwise's Authoring Tool Communication");
        return false;
    }
#endif // AK_OPTIMIZED

    return true;

}


bool AudioSystem::InitMemoryManager() {

    AkMemSettings memSettings;
    AK::MemoryMgr::GetDefaultSettings(memSettings);

    //Init memory manager
    if (AK::MemoryMgr::Init(&memSettings) != AK_Success)
    {
        LOG_CONSOLE("Could not create the memory manager.");
        return false;
    }

    return true;
}

bool AudioSystem::InitStreamingManager() {

    wchar_t cwd[MAX_PATH];
    _wgetcwd(cwd, MAX_PATH);
    LOG_CONSOLE("Current Working Directory: %ls", cwd);
    
    AkStreamMgrSettings stmSettings;
    AK::StreamMgr::GetDefaultSettings(stmSettings);

    if (!AK::StreamMgr::Create(stmSettings)) return false;

    AkDeviceSettings deviceSettings;
    AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);

    // Initializing the Deferred hook
    if (g_lowLevelIO.Init(deviceSettings) != AK_Success) return false;

    g_lowLevelIO.SetBasePath(L"..\\Assets\\Audio\\GeneratedSoundBanks\\Windows\\");

    //const wchar_t* finalPath = L"..\\Assets\\Audio\\GeneratedSoundBanks\\Windows\\";
    //g_lowLevelIO.SetBasePath(finalPath);

    return true;
}

bool AudioSystem::InitSoundEngine()
{
    //Sound Engine creation using default initialization parameters

    AkInitSettings initSettings;
    AkPlatformInitSettings platformInitSettings;
    AK::SoundEngine::GetDefaultInitSettings(initSettings);
    AK::SoundEngine::GetDefaultPlatformInitSettings(platformInitSettings);

    if (AK::SoundEngine::Init(&initSettings, &platformInitSettings) != AK_Success)
    {
        LOG_CONSOLE("Could not initialize the Sound Engine.");
        return false;
    }

    return true;

}

bool AudioSystem::InitSpatialAudio() {

    //Spatial Audio initalization using default initialization parameters
    AkSpatialAudioInitSettings settings; // The constructor fills AkSpatialAudioInitSettings with the recommended default settings. 
    if (AK::SpatialAudio::Init(settings) != AK_Success)
    {
        LOG_CONSOLE("Could not initialize the Spatial Audio.");
        return false;
    }


    return true;
}

bool AudioSystem::InitCommunication() {

    ////Necessary if we want to be able to connect the Wwise Authoring tool to the game and perform in-game mixing, profiling, and troubleshooting.

    #ifndef AK_OPTIMIZED
    // Initialize communications (not in release build!)

    AkCommSettings commSettings;
    AK::Comm::GetDefaultInitSettings(commSettings);
    if (AK::Comm::Init(commSettings) != AK_Success)
    {
        LOG_CONSOLE("Could not initialize communication.");
        return false;
    }
    #endif // AK_OPTIMIZED

    return true;
}

bool AudioSystem::Awake() {

	//Check engine has successfully initialized
    if (!InitEngine()) {
        LOG_CONSOLE("Failed to initialized the Audio Engine");
        return false;
    }
    else {
        LOG_CONSOLE("Successfully initialized Audio Engine");
    }

    LOG_CONSOLE("Header Version: %d", AK_WWISESDK_VERSION_MAJOR);
    LOG_CONSOLE("Build Date: %s", __DATE__);

    //create audio events
    for (size_t i = 0; i < MAX_AUDIO_EVENTS; ++i) {

        audioEvents.push_back(new AudioEvent);
    }

    //load init bank
    LoadBank(BANKNAME_INIT);

    //load project specific bank
    LoadBank(BANKNAME_MUSIC);

    //event list for the UI dropdown
    DiscoverEvents();

    SetGlobalVolume(globalVolume);
    return true;
}

bool AudioSystem::Update() { 
    //components push their current position to Wwise
    for (auto* component : audioComponents) {
        component->SetTransform();
    }

    //ProcessAudio() in the Sound Integration Walkthrough
    //processes bank requests, events, positions, RTPC, etc.
    AK::SoundEngine::RenderAudio();
    return true;
}

bool AudioSystem::CleanUp() {
    //Stop all audio and unload banks first
    AK::SoundEngine::StopAll();
    AK::SoundEngine::ClearBanks(); // Always clear banks before termination
    AK::SoundEngine::RenderAudio(); // Process the stop/clear commands

    // Terminate Communication FIRST 
#ifndef AK_OPTIMIZED
    AK::Comm::Term();
#endif

    // Terminate Sound Engine
    AK::SoundEngine::Term();

    g_lowLevelIO.Term();

    // Terminate Stream Manager
    if (AK::IAkStreamMgr::Get()) {
        AK::IAkStreamMgr::Get()->Destroy();
    }

    // Terminate Memory Manager LAST
    AK::MemoryMgr::Term();

    return true;

}

void AudioSystem::PlayEvent(AkUniqueID event, AkGameObjectID goID)
{
    for (size_t i = 0; i < MAX_AUDIO_EVENTS; i++)
    {
        //grab the first available slot (0L) to play an event in the max events pool 
        if (audioEvents[i]->playingID == 0L)
        {
            AK::SoundEngine::PostEvent(event, goID, AkCallbackType::AK_EndOfEvent, audioEvents[i]->eventCallback, (void*)audioEvents[i]);
            LOG_DEBUG("Playing event from %d audiogameobject", goID);
            audioEvents[i]->playingID = 1L; //1L = event slot is now taken

            return;
        }
    }
    LOG_DEBUG("Maximum amount of audio events at the same time reached: %d", MAX_AUDIO_EVENTS);
}


// wrapper for string version of PlayEvent
void AudioSystem::PlayEvent(const wchar_t* eventName, AkGameObjectID goID)
{
    AkUniqueID eventID = AK::SoundEngine::GetIDFromString(eventName);

    if (eventID == AK_INVALID_UNIQUE_ID)
    {
        LOG_CONSOLE("Wwise Error: Event name '%ls' not found!", eventName);
        return;
    }

    PlayEvent(eventID, goID);
}

void AudioSystem::StopEvent(AkUniqueID event, AkGameObjectID goID) {
    AK::SoundEngine::ExecuteActionOnEvent(event, AK::SoundEngine::AkActionOnEventType::AkActionOnEventType_Stop, goID);
    LOG_DEBUG("Stopping event from %d audiogameobject", goID);
}

void AudioSystem::PauseEvent(AkUniqueID event, AkGameObjectID goID) {
    AK::SoundEngine::ExecuteActionOnEvent(event, AK::SoundEngine::AkActionOnEventType::AkActionOnEventType_Pause, goID);
    LOG_DEBUG("Pausing event from %d audiogameobject", goID);
}

void AudioSystem::ResumeEvent(AkUniqueID event, AkGameObjectID goID) {
    AK::SoundEngine::ExecuteActionOnEvent(event, AK::SoundEngine::AkActionOnEventType::AkActionOnEventType_Resume, gameObjectIDs[goID]);
    LOG_DEBUG("Resuming event from %d audiogameobject", goID);
}

void AudioSystem::SetState(AkStateGroupID stateGroup, AkStateID state)
{
    AK::SoundEngine::SetState(stateGroup, state);
    LOG_DEBUG("Setting wwise state through ID");
}

void AudioSystem::SetState(const char* stateGroup, const char* state)
{
    AK::SoundEngine::SetState(stateGroup, state);
    LOG_DEBUG("Setting wwise state through name");
}


void AudioSystem::SetSwitch(AkSwitchGroupID switchGroup, AkSwitchStateID switchState, AkGameObjectID goID)
{
    AK::SoundEngine::SetSwitch(switchGroup, switchState, goID);
    LOG_DEBUG("Setting wwise switch");
}

void AudioSystem::SetRTPCValue(AkRtpcID rtpcID, AkRtpcValue value) {
    AK::SoundEngine::SetRTPCValue(rtpcID, value);
    LOG_DEBUG("Setting RTPC value through ID");
}

void AudioSystem::SetRTPCValue(const char* name, int value) {
    AK::SoundEngine::SetRTPCValue(name, value);
    LOG_DEBUG("Setting RTPC value through name");
}


void AudioSystem::SetGlobalVolume(float vol) {
    if (vol < 0.0f) vol = 0.0f;
    else if (vol > 100.0f) vol = 100.0f;

    globalVolume = vol;
    AK::SoundEngine::SetOutputVolume(AK::SoundEngine::GetOutputID(AK_INVALID_UNIQUE_ID, 0.0f), (AkReal32)(vol * 0.01f)); //because wwise maps volume 0 to 1!
}

void AudioSystem::SetMasterVolume(int vol) {
    AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::MASTER_VOLUME, (AkRtpcValue)vol);
}

void AudioSystem::SetMusicVolume(int vol) {
    AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::MUSIC_VOLUME, (AkRtpcValue)vol);
}

void AudioSystem::SetSFXVolume(int vol) {
    AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::SFX_VOLUME, (AkRtpcValue)vol);
}
//
//void AudioSystem::SetDialogVolume(int vol) {
//    AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::DIALOG_VOLUME, (AkRtpcValue)vol);
//}


void AudioSystem::LoadBank(const wchar_t* bankName) {
    AkBankID bankID;
    AKRESULT eResult = AK::SoundEngine::LoadBank(bankName, bankID);

    if (eResult == AK_Success) {
        LOG_CONSOLE("Loaded Bank: %ls", bankName);
    }
    else {
        LOG_CONSOLE("Wwise Bank Error: %ls (Result Code: %d)", bankName, (int)eResult);
    }
}

void AudioSystem::RegisterAudioComponent(AudioComponent* component) {
    audioComponents.push_back(component);
}

void AudioSystem::UnregisterAudioComponent(AudioComponent* component) {
    auto it = std::find(audioComponents.begin(), audioComponents.end(), component);
    if (it != audioComponents.end()) {
        audioComponents.erase(it);
    }
}

void AudioSystem::RegisterGameObject(AkGameObjectID id, const char* name) {
    AK::SoundEngine::RegisterGameObj(id, name);
}

void AudioSystem::UnregisterGameObject(AkGameObjectID id) {
    AK::SoundEngine::UnregisterGameObj(id);
}

void AudioSystem::SetPosition(AkGameObjectID id, const glm::vec3& pos, const glm::vec3& front, const glm::vec3& top) {
    AkSoundPosition soundPos;
    soundPos.SetPosition(pos.x, pos.y, pos.z);
    soundPos.SetOrientation(front.x, front.y, front.z, top.x, top.y, top.z);
    AK::SoundEngine::SetPosition(id, soundPos);
}



void AudioSystem::EventCallBack(AkCallbackType in_eType, AkEventCallbackInfo* in_pEventInfo, void* in_pCallbackInfo, void* in_pCookie)
{
    // in this version of Wwise, the 'cookie' is passed as the 4th parameter
    // this is the pointer to the AudioEvent passed in PostEvent
    AudioEvent* pEvent = (AudioEvent*)in_pCookie;

    if (pEvent && in_eType == AkCallbackType::AK_EndOfEvent)
    {
        pEvent->playingID = 0L; 
    }
}

void AudioSystem::DiscoverEvents() {
    // Clear existing names to avoid duplicates
    eventNames.clear();
    std::string path = "..\\Assets\\Audio\\GeneratedSoundBanks\\Windows\\MainSoundBank.json";

    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_CONSOLE("Audio Error: Could not open MainSoundBank.json at %s", path.c_str());
        return;
    }

    try {
        nlohmann::json data;
        file >> data;

        //SoundBanksInfo -> SoundBanks
        if (data.contains("SoundBanksInfo") && data["SoundBanksInfo"].contains("SoundBanks")) {
            for (auto& bank : data["SoundBanksInfo"]["SoundBanks"]) {
                // Check the "Events" array
                if (bank.contains("Events")) {
                    for (auto& event : bank["Events"]) {
                        if (event.contains("Name")) {
                            std::string name = event["Name"].get<std::string>();
                            eventNames.push_back(name);
                        }
                    }
                }
            }
        }

        std::sort(eventNames.begin(), eventNames.end());
        LOG_CONSOLE("Audio: Discovered %d events from MainSoundBank.json", (int)eventNames.size());
    }
    catch (const nlohmann::json::exception& e) {
        LOG_CONSOLE("Audio Error: Failed to parse MainSoundBank.json: %s", e.what());
    }
}

void AudioSystem::StopAllAudio() {
    AK::SoundEngine::StopAll();
    AK::SoundEngine::RenderAudio();
}

//---------------AK and helpers

namespace AK
{
    void ConvertFileIdToFilename(AkOSChar* out_pszTitle, AkUInt32 in_pszTitleMaxLen, AkUInt32 in_codecId, AkFileID in_fileID)
    {
        // This handles loading by ID (e.g., 1355168291.bnk)
        swprintf(out_pszTitle, in_pszTitleMaxLen, AKTEXT("%u.bnk"), (unsigned int)in_fileID);
    }

    bool ResolveGeneratedSoundBanksPath(StringBuilder& dest, const AkOSChar* in_pszFileName, AkFileSystemFlags* in_pFlags, bool bUseSubfoldering)
    {
        //append name to the dest (which already contains the BasePath)
        if (in_pszFileName)
        {
            return dest.Append(in_pszFileName);
        }
        return true;
    }
}