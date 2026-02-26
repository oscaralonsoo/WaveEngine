#pragma once
#include "Joint.h"

class DistanceJoint : public Joint {
public:
    DistanceJoint(GameObject* owner);
    ~DistanceJoint();

    bool IsType(ComponentType type) override { return type == ComponentType::JOINT || type == ComponentType::DISTANCE_JOINT; };

    void CreateJoint() override;

    void SetMaxDistance(float mD);
    float GetMaxDistance() const { return maxDistance; }
    void EnableMaxDistance(bool b);
    bool GetMaxDistanceEnabled() const { return maxDistanceEnabled; }
    void SetMinDistance(float mD);
    float GetMinDistance() const { return minDistance; }
    void EnableSpring(bool b);
    bool GetSpringEnabled() const { return springEnabled; }
    void SetStiffness(float s);
    float GetStiffness() const { return stiffness; }
    void SetDamping(float d);
    float GetDamping() const { return damping; }

    virtual void Serialize(nlohmann::json& componentObj) const;
    virtual void Deserialize(const nlohmann::json& componentObj);

    void DrawDebug() override;
    void OnEditor() override;

    //void Serialize(nlohmann::json& componentObj) const override;
    //void Deserialize(const nlohmann::json& componentObj) override;


private:
    float maxDistance = 10.0f;
    bool maxDistanceEnabled = true;
    float minDistance = 0.0f;
    bool springEnabled = false;
    float stiffness = 0.0f;
    float damping = 0.0f;
};