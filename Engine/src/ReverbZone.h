#pragma once

#include "Component.h"
#include <string>
#include <glm/glm.hpp>
#include "../Assets/Audio/GeneratedSoundBanks/Wwise_IDs.h"

struct ReverbPresetData {
    AkUniqueID busID;
    std::string name;
};



extern const std::vector<ReverbPresetData> reverbPresets;

class ReverbZone : public Component {
public:
    enum class Shape {
        SPHERE = 0,
        BOX = 1
    };   

    ReverbZone(GameObject* owner);
    virtual ~ReverbZone();

    std::string GetBusNameFromID(AkUniqueID id) const;
    AkUniqueID GetIDFromBusName(const std::string& name) const;
   

    // point test in world space
    bool ContainsPoint(const glm::vec3& worldPoint) const;

    // Component overrides
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;
    void OnEditor() override;

public:
    Shape shape = Shape::SPHERE;
    int presetIndex = 1; // Set to cathedral by default
    float radius = 1.0f;                // sphere radius (world units)
    glm::vec3 extents = glm::vec3(5.0f); // box half-extents (local)

    std::string auxBusName = "Reverb_Cathedral"; // Wwise Aux Bus name
    AkUniqueID auxBusID = AK::AUX_BUSSES::REVERB_CATHEDRAL;
    //std::string reverbPresetName = "Cathedral";
    float wetLevel = 1.0f;              // 0.0 - 1.0
    int priority = 0;                   // higher priority wins when overlapping
    bool enabled = true;
};