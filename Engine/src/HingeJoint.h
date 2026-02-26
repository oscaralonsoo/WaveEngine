#pragma once
#include "Joint.h"

class HingeJoint : public Joint {
public:
    HingeJoint(GameObject* owner);
    virtual ~HingeJoint();

    bool IsType(ComponentType type) override { return type == ComponentType::JOINT || type == ComponentType::HINGE_JOINT; };

    void CreateJoint() override;

    void EnableLimits(bool b);
    void SetMinAngle(float angle);
    void SetMaxAngle(float angle);

    void EnableMotor(bool b);
    void SetDriveVelocity(float v);

    virtual void Serialize(nlohmann::json& componentObj) const;
    virtual void Deserialize(const nlohmann::json& componentObj);

    //void Save(Config& config) override;
    //void Load(Config& config) override;
    void OnEditor() override;
    void DrawDebug() override;

    //void Serialize(nlohmann::json& componentObj) const override;
    //void Deserialize(const nlohmann::json& componentObj) override;

private:

    float minAngle = -45.0f;
    float maxAngle = 45.0f;
    bool limitsEnabled = false;

    float driveVelocity = 0.0f;
    bool motorEnabled = false;
};