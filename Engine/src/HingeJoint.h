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

    float GetMinAngle() const { return minAngle; }
    float GetMaxAngle() const { return maxAngle; }
    bool GetLimitsEnabled() const { return limitsEnabled; }
    float GetDriveVelocity() const { return driveVelocity; }
    bool GetMotorEnabled() const { return motorEnabled; }

    void OnEditor() override {}
    void DrawDebug() override;

private:
    float minAngle = -45.0f;
    float maxAngle = 45.0f;
    bool limitsEnabled = false;

    float driveVelocity = 0.0f;
    bool motorEnabled = false;
};