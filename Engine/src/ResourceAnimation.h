#pragma once
#include "ModuleResources.h"
#include "ModuleLoader.h"

struct Channel
{
    std::string name;

    std::vector<glm::vec3> positionKeys;
    std::vector<glm::quat> rotationKeys;
    std::vector<glm::vec3> scaleKeys;
};

struct Animation {
    double duration = 0.0;
    double ticksPerSecond = 0.0;
    std::vector<Channel> channels;

    bool IsValid() const {
        return !channels.empty();
    }
};

class ResourceAnimation : public Resource {
public:
    ResourceAnimation(UID uid);
    virtual ~ResourceAnimation();

    bool LoadInMemory() override;
    void UnloadFromMemory() override;

    const double GetDuration() const { return animation.duration; }
    const double GetTicksPerSecond() const { return animation.ticksPerSecond; }
    const std::vector<Channel>& GetChanels() const { return animation.channels; }
    const bool GetAnimationIsValid() const { return animation.IsValid(); }

private:
    
    Animation animation;
};