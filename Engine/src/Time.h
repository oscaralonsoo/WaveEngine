#pragma once
#include "Module.h"

class Time : public Module
{
public:
	Time();
	~Time();

	bool Start() override;
	bool PreUpdate() override;
	bool PostUpdate() override;

	float GetDeltaTime() const { return gameDeltaTime; }
	float GetRealDeltaTime() const { return deltaTime; }
	float GetTotalTime() const { return totalTime; }
	float GetGameTime() const { return gameTime; }
	float GetTimeScale() const { return timeScale; }

	void Pause() { isPaused = true; }
	void Resume() { isPaused = false; }
	void Reset();
	void SetTimeScale(float scale) { timeScale = scale > 0.0f ? scale : 1.0f; }
	void StepFrame() { shouldStepFrame = true; }

	bool IsPaused() const { return isPaused; }

private:
	float deltaTime;
	float gameDeltaTime;
	float totalTime;
	float gameTime;
	float lastFrame;
	bool isPaused;
	float timeScale;
	bool shouldStepFrame;
};