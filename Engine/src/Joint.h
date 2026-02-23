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
    void SetBreakTorque(float torque = INFINITY_PHYSIC);

    // Getters for Inspector
    Rigidbody* GetBodyA() const { return bodyA; }
    Rigidbody* GetBodyB() const { return bodyB; }
    glm::vec3 GetLocalPosA() const { return localPosA; }
    glm::vec3 GetLocalPosB() const { return localPosB; }
    glm::quat GetLocalRotA() const { return localRotA; }
    glm::quat GetLocalRotB() const { return localRotB; }
    float GetBreakForce() const { return breakForce; }
    float GetBreakTorque() const { return breakTorque; }

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