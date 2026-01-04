#pragma once
#include "Module.h"
#include "AudioUtility.h"

#define MAX_AUDIO_EVENTS 20

class AudioEvent {
public:
	AudioEvent();

	bool IsPlaying();

	AkPlayingID playingID;
	AkCallbackFunc eventCallback; //callback function wwise uses to fire events
};

class AudioSystem : public Module {
public:

	AudioSystem();
	~AudioSystem();

	bool Awake();

	bool Update();

	bool CleanUp();

	void SetGlobalVolume(float volume) { globalVolume = volume; }
	float GetGlobalVolume() { return globalVolume; }

private:
	bool InitEngine();
	bool InitMemoryManager();
	bool InitStreamingManager();
	bool InitSoundEngine();
	bool InitSpatialAudio();
	bool InitCommunication();

	float globalVolume = 100.0f;

	std::vector<AkGameObjectID> gameObjectIDs;
	std::vector<AudioEvent*> audioEvents;

public:
	//low_level I/O implementation taken from the samples folder
	CAkFilePackageLowLevelIODeferred g_lowLevelIO;

	//implements AK::StreamMgr::IAkFileLocationResolver + AK::StreamMgr::IAkLowLevelIOHook interfaces, 
	//and is able to load file packages generated with the File Packager utility

};
