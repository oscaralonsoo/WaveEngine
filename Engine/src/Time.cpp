#include "Time.h"
#include "Log.h"
#include <SDL3/SDL.h>

Time* Time::instance = nullptr;

Time::Time() : Module(), deltaTime(0.0f), gameDeltaTime(0.0f), totalTime(0.0f), gameTime(0.0f), lastFrame(0.0f), isPaused(true), timeScale(1.0f), shouldStepFrame(false)
{
	instance = this;  
}

Time::~Time()
{
	instance = nullptr;  
}

bool Time::Start()
{
	lastFrame = SDL_GetTicks() / 1000.0f;
	return true;
}

bool Time::PreUpdate()
{
	float currentFrame = SDL_GetTicks() / 1000.0f;
	deltaTime = currentFrame - lastFrame;
	totalTime = currentFrame;
	lastFrame = currentFrame;

	// Calculate game delta time based on pause state
	if (isPaused && !shouldStepFrame)
	{
		gameDeltaTime = 0.0f;
	}
	else
	{
		gameDeltaTime = deltaTime * timeScale;
		if (shouldStepFrame)
		{
			shouldStepFrame = false;
		}
	}

	gameTime += gameDeltaTime;

	return true;
}

bool Time::PostUpdate()
{
	return true;
}

void Time::Reset()
{
	lastFrame = SDL_GetTicks() / 1000.0f;
	totalTime = 0.0f;
	gameTime = 0.0f;
	deltaTime = 0.0f;
	isPaused = false;
}

float Time::GetDeltaTimeStatic()
{
	if (!instance) return 0.016f;
	return instance->gameDeltaTime;
}

float Time::GetRealDeltaTimeStatic()
{
	if (!instance) return 0.016f;
	return instance->deltaTime;
}

float Time::GetTotalTimeStatic()
{
	if (!instance) return 0.0f;
	return instance->totalTime;
}