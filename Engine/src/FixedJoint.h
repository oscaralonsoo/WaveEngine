#pragma once
#include "Joint.h"

class FixedJoint : public Joint {
public:
    FixedJoint(GameObject* owner);
    virtual ~FixedJoint();

    bool IsType(ComponentType type) override { return type == ComponentType::JOINT || type == ComponentType::FIXED_JOINT; };

    void CreateJoint() override;

    virtual void Serialize(nlohmann::json& componentObj) const;
    virtual void Deserialize(const nlohmann::json& componentObj);

    //void Save(Config& config) override;
    //void Load(Config& config) override;
    void OnEditor() override;
    void DrawDebug() override;

    //void Serialize(nlohmann::json& componentObj) const override;
    //void Deserialize(const nlohmann::json& componentObj) override;
};