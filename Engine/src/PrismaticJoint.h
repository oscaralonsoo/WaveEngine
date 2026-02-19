#pragma once
#include "Joint.h"

class PrismaticJoint : public Joint {
public:
    PrismaticJoint(GameObject* owner);
    virtual ~PrismaticJoint();

    bool IsType(ComponentType type) override { return type == ComponentType::PRISMATIC_JOINT || type == ComponentType::JOINT; };

    void CreateJoint() override;

    void EnableLimits(bool b);
    void SetMinLimit(float m);
    void SetMaxLimit(float m);

    void EnableSoftLimit(bool b);
    void SetStiffness(float s);
    void SetDamping(float d);

    //void Save(Config& config) override;
    //void Load(Config& config) override;
    void OnEditor() override;
    void DrawDebug() override;

private:
    float minLimit = -5.0f;
    float maxLimit = 5.0f;
    bool limitsEnabled = false;

    float stiffness = 0.0f;
    float damping = 0.0f;
    bool softLimitEnabled = false;
};