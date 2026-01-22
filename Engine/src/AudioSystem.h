#pragma once
#include "Module.h"
#include "AudioUtility.h"
#include <vector>
#include <algorithm>
#include <Win32/AkDefaultIOHookDeferred.h>

class AudioComponent;

#define MAX_AUDIO_EVENTS 30

class AudioEvent {
public:
	AudioEvent();

	bool IsEventPlaying() {
		return playingID != 0;
	};

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


	void PlayEvent(AkUniqueID event, AkGameObjectID goID);

	void PlayEvent(const wchar_t* eventName, AkGameObjectID goID);
	void StopEvent(AkUniqueID event, AkGameObjectID goID);
	void PauseEvent(AkUniqueID event, AkGameObjectID goID);
	void ResumeEvent(AkUniqueID event, AkGameObjectID goID);

	// ----------------------- STATES ---------------------- //
	void SetState(AkStateGroupID stateGroup, AkStateID state);
	void SetState(const char* stateGroup, const char* state);

	// ---------------------- SWITCHES ---------------------- //
	void SetSwitch(AkSwitchGroupID switchGroup, AkSwitchStateID switchState, AkGameObjectID goID);

	// ------------------------ RTPC ------------------------ //
	void SetRTPCValue(const char* name, int value);
	void SetRTPCValue(AkRtpcID rtpcID, AkRtpcValue value);

	/* (From Wwise Docs) On playback, if no such game object-specific value has been set, 
	the sound engine will look for a global RTPC value that will have been set by calling 
	AK::SoundEngine::SetRTPCValue() with the AK_INVALID_GAME_OBJECT parameter or with no specified game object. 
	If this global value does not exist, the sound engine will use the 
	Game Parameter's default value as specified in the Wwise authoring application.
	
	Therefore these methods are for global parameters set in WWiseProject
	
	*/

	////audio engine functions
	//void PlayEngine();
	//void PauseEngine();
	//void StopEngine();


	void SetGlobalVolume(float volume);
	float GetGlobalVolume() { return globalVolume; }
	
	void SetMasterVolume(int volume);
	/*void SetDialogVolume(int volume);*/
	void SetSFXVolume(int volume);
	void SetMusicVolume(int volume);

	void RegisterAudioComponent(AudioComponent* component);
	void UnregisterAudioComponent(AudioComponent* component);

	void RegisterGameObject(AkGameObjectID id, const char* name);
	void UnregisterGameObject(AkGameObjectID id);
	void SetPosition(AkGameObjectID id, const glm::vec3& pos, const glm::vec3& front, const glm::vec3& top);

	static void EventCallBack(AkCallbackType in_eType, AkEventCallbackInfo* in_pEventInfo, void* in_pCallbackInfo, void* in_pCookie);

	void DiscoverEvents();
	void StopAllAudio();

private:
	bool InitEngine();
	bool InitMemoryManager();
	bool InitStreamingManager();
	bool InitSoundEngine();
	bool InitSpatialAudio();
	bool InitCommunication();

	void AudioSystem::LoadBank(const wchar_t* bankName);

	

	float globalVolume = 100.0f;

	std::vector<AkGameObjectID> gameObjectIDs;
	std::vector<AudioEvent*> audioEvents;

public:
	//low_level I/O implementation taken from the samples folder
	CAkDefaultIOHookDeferred g_lowLevelIO;

	//implements AK::StreamMgr::IAkFileLocationResolver + AK::StreamMgr::IAkLowLevelIOHook interfaces, 
	//and is able to load file packages generated with the File Packager utility

	//list of events
	std::vector<std::string> eventNames;

	std::vector<AudioComponent*> audioComponents;

};
