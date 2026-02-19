#pragma once
#include "Component.h"
#include "ModulePhysics.h"
#include <physx/PxPhysicsAPI.h>
#include "glm/glm.hpp"
#include <glm/gtc/quaternion.hpp>


class Rigidbody;

class Joint : public Component {
public:
    
    enum class JointBody {
        Self = 0,
        Target = 1
    };

    Joint(GameObject* owner);
    virtual ~Joint() {};

    virtual void CleanUp();
    virtual void Update();

    virtual void DrawDebug() = 0;

    void OnRigidbodyReset(Rigidbody* rb);
    void OnRigidbodyDeleted(Rigidbody* rb);

    virtual void CreateJoint() = 0;
    virtual void DestroyJoint();
    virtual void RefreshJoint();

    virtual bool IsType(ComponentType type) override = 0;
    bool IsIncompatible(ComponentType type) override {
        return false;
    };

    void SetTarget(GameObject* targetGO);
    void SetAnchorPosition(JointBody jointBodyType, const glm::vec3& position);
    void SetAnchorRotation(JointBody jointBodyType, const glm::quat& rotation);
    void SetBreakForce(float force = INFINITY_PHYSIC);
    void SetBreakTorque(float torque= INFINITY_PHYSIC);

    //virtual void Save(Config& componentNode) {}
    //void SaveBase(Config& config);
    //virtual void Load(Config& componentNode) {}
    //void LoadBase(Config& config);
    //void ResolveReferences() override;

    void OnEditorBase();
    void OnGameObjectEvent(GameObjectEvent event, Component* component) override;


protected:
    
    uint32_t bUID;

    Rigidbody* bodyA = nullptr;
    Rigidbody* bodyB = nullptr;

    physx::PxJoint* pxJoint = nullptr;

    glm::vec3 localPosA;
    glm::quat localRotA;

    glm::vec3 localPosB;
    glm::quat localRotB;

    float breakForce = INFINITY_PHYSIC;
    float breakTorque = INFINITY_PHYSIC;

    void SyncFrames();
};