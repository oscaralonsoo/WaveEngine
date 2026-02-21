#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ResourceAnimation.h"

struct aiAnimation;

class AnimationImporter {
public:
    AnimationImporter();
    ~AnimationImporter();

    // IMPORT: Convert from Assimp animation to our AnimationData structure
    static Animation ImportFromAssimp(const aiAnimation* aiAnimation);

    // SAVE: Save our AnimationData to custom binary format in Library/Animations/
    static bool SaveToCustomFormat(const Animation& animation, const std::string& filename);

    // LOAD: Load from custom binary format back into our AnimationData structure
    static Animation LoadFromCustomFormat(const std::string& filename);
};