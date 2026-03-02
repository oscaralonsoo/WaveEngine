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

    virtual void Serialize(nlohmann::json& componentObj) const;
    virtual void Deserialize(const nlohmann::json& componentObj);

    //void Save(Config& config) override;
    //void Load(Config& config) override;
    void OnEditor() override;
    void DrawDebug() override;

    //void Serialize(nlohmann::json& componentObj) const override;
    //void Deserialize(const nlohmann::json& componentObj) override;

private:
    physx::PxD6Motion::Enum motions[6];

    float linearLimit = 1.0f;
    float twistMin = -45.0f, twistMax = 45.0f;
    float swingY = 45.0f, swingZ = 45.0f;
};