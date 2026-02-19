#pragma once
#include "Joint.h"

class FixedJoint : public Joint {
public:
    FixedJoint(GameObject* owner);
    virtual ~FixedJoint();

    bool IsType(ComponentType type) override { return type == ComponentType::JOINT || type == ComponentType::FIXED_JOINT; };

    void CreateJoint() override;

    //void Save(Config& config) override;
    //void Load(Config& config) override;
    void OnEditor() override;
    void DrawDebug() override;
};