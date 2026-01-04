#include "AudioSystem.h"
#include "Log.h"

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
    
    // Create & initialize instance of the default streaming manager (could be overridden with our own, if we had one)

    AkStreamMgrSettings stmSettings;
    AK::StreamMgr::GetDefaultSettings(stmSettings);

    // Customize Stream Manager settings if needed

    if (!AK::StreamMgr::Create(stmSettings))
    {
        LOG_CONSOLE("Could not create the Streaming Manager");
        return false;
    }

    //create streaming device (could also overridden with our own, if we had one)
    AkDeviceSettings deviceSettings;
    AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);

    // Customize the streaming device settings here.

    // CAkFilePackageLowLevelIODeferred::Init() creates a streaming device
    // in the Stream Manager, and registers itself as the File Location Resolver.
    
    if (g_lowLevelIO.Init(deviceSettings) != AK_Success)
    {
        LOG_CONSOLE("Could not create the streaming device and Low-Level I/O system");
        return false;
    }

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

    //Necessary if we want to be able to connect the Wwise Authoring tool to the game and perform in-game mixing, profiling, and troubleshooting.

    #ifndef AK_OPTIMIZED
    // Initialize communications (not in release build!)
    //
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

	//create audio events
	for (size_t i = 0; i < MAX_AUDIO_EVENTS; ++i) {
        
        audioEvents.push_back(new AudioEvent);
	}

    SetGlobalVolume(globalVolume);
}

bool AudioSystem::Update() { 
    //ProcessAudio() in the Sound Integration Walkthrough
    //processes bank requests, events, positions, RTPC, etc.
    AK::SoundEngine::RenderAudio();
}

bool AudioSystem::CleanUp() {
    //Free audioevents memory
    for (const auto event : audioEvents) delete event;
    audioEvents.clear();

    //terminate audio system's submodules (in reverse creation order)
#ifndef AK_OPTIMIZED
    AK::Comm::Term();
#endif // AK_OPTIMIZED

    //Spatial Audio doesn't require manual termination
    AK::SoundEngine::Term();

    g_lowLevelIO.Term();
    // CAkFilePackageLowLevelIODeferred::Term() destroys its associated streaming device 
    // that lives in the Stream Manager, and unregisters itself as the File Location Resolver.
    
    //StreamManager has a different termination function
    if (AK::IAkStreamMgr::Get()) AK::IAkStreamMgr::Get()->Destroy();

    AK::MemoryMgr::Term();

}