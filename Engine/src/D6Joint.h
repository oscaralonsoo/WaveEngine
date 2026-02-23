#pragma once
#include "Joint.h"

class D6Joint : public Joint {
public:
    D6Joint(GameObject* owner);
    virtual ~D6Joint();

    bool IsType(ComponentType type) override { return type == ComponentType::JOINT || type == ComponentType::D6_JOINT; };

    void CreateJoint() override;

    void SetMotion(physx::PxD6Axis::Enum axis, physx::PxD6Motion::Enum motion);
    void SetLinearLimit(float extent);
    void SetTwistLimit(float minAngle, float maxAngle);
    void SetSwingLimit(float yAngle, float zAngle);

    physx::PxD6Motion::Enum GetMotion(int axis) const { return motions[axis]; }
    float GetLinearLimit() const { return linearLimit; }
    float GetTwistMin() const { return twistMin; }
    float GetTwistMax() const { return twistMax; }
    float GetSwingY() const { return swingY; }
    float GetSwingZ() const { return swingZ; }

    void OnEditor() override {}
    void DrawDebug() override;

private:
    physx::PxD6Motion::Enum motions[6];

    float linearLimit = 1.0f;
    float twistMin = -45.0f, twistMax = 45.0f;
    float swingY = 45.0f, swingZ = 45.0f;
};