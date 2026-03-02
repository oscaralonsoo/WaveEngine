#pragma once
#include "Joint.h"

class SphericalJoint : public Joint {
public:
    SphericalJoint(GameObject* owner);
    virtual ~SphericalJoint();

    bool IsType(ComponentType type) override { return type == ComponentType::JOINT || type == ComponentType::SPHERICAL_JOINT; };

    void CreateJoint() override;

    void EnableLimits(bool b);
    void SetConeLimit(float angle);

    virtual void Serialize(nlohmann::json& componentObj) const;
    virtual void Deserialize(const nlohmann::json& componentObj);

    //void Save(Config& config) override;
    //void Load(Config& config) override;
    void OnEditor() override;
    void DrawDebug() override;

    //void Serialize(nlohmann::json& componentObj) const override;
    //void Deserialize(const nlohmann::json& componentObj) override;

private:
    float limitAngle = 45.0f;
    bool limitsEnabled = false;
};