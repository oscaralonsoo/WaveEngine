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

    //void Save(Config& config) override;
    //void Load(Config& config) override;
    void OnEditor() override;
    void DrawDebug() override;

private:
    float limitAngle = 45.0f;
    bool limitsEnabled = false;
};