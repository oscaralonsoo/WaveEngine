#pragma once
#include "Component.h"
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

class ComponentCollider : public Component {
public:
    ComponentCollider(GameObject* owner, ComponentType type);
    virtual ~ComponentCollider();

    virtual btCollisionShape* GetShape() const { return shape; }
    
    void SetTrigger(bool isTrigger);
    bool IsTrigger() const { return trigger; }

protected:
    btCollisionShape* shape = nullptr;
    bool trigger = false;
    glm::vec3 offset = glm::vec3(0.0f);
};