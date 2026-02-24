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
   
    bool IsType(ComponentType type) override { return type == ComponentType::REVERBZONE; };
    bool IsIncompatible(ComponentType type) override { return false; };

    // point test in world space
    bool ContainsPoint(const glm::vec3& worldPoint) const;

    // Component overrides
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;
    void OnEditor() override;

public:
    Shape shape = Shape::SPHERE;
    int presetIndex = 1;
    float radius = 1.0f;
    glm::vec3 extents = glm::vec3(5.0f);

    std::string auxBusName = "Reverb_Cathedral";
    AkUniqueID auxBusID = AK::AUX_BUSSES::REVERB_CATHEDRAL;
    //std::string reverbPresetName = "Cathedral";
    float wetLevel = 1.0f;
    int priority = 0;
    bool enabled = true;
};