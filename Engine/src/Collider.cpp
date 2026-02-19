#include "Collider.h"
#include "Rigidbody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "GameObject.h"
#include "Transform.h"
#include "imgui.h"
#include "glm/glm.hpp"

Collider::Collider(GameObject* owner) : Component(owner, ComponentType::COLLIDER)
{
    name = "Collider";
    Rigidbody* rb = (Rigidbody*)owner->GetComponentInParent(ComponentType::RIGIDBODY);
    if (rb) rb->CreateBody();
}

Collider::~Collider()
{
    if (attachedRigidbody) attachedRigidbody->UnattachCollider(this);
}


void Collider::Enable() 
{
    if (attachedRigidbody) {
        attachedRigidbody->CreateBody();
    }
    else {
        Rigidbody* rb = (Rigidbody*)owner->GetComponentInParent(ComponentType::RIGIDBODY);
        if (rb) rb->CreateBody();
    }
}

void Collider::Disable() 
{
    if (attachedRigidbody) attachedRigidbody->UnattachCollider(this);
}


//void Collider::SaveBase(Config& config)
//{
//    config.SetVector3("Center", center);
//    config.SetBool("Trigger", isTrigger);
//    config.SetFloat("DynamicFriction", dynamicFriction);
//    config.SetFloat("StaticFriction", staticFriction);
//    config.SetFloat("Restitution", restitution);
//}
//
//void Collider::LoadBase(Config& config)
//{
//    center = config.GetVector3("Center");
//    isTrigger = config.GetBool("Trigger");
//    dynamicFriction = config.GetFloat("DynamicFriction");
//    staticFriction = config.GetFloat("StaticFriction");
//    restitution = config.GetFloat("Restitution");
//}

void Collider::OnEditorBase()
{
    //ATRIBUTES
    ImGui::Text("Trigger");
    bool trigger = isTrigger;
    if (ImGui::Checkbox("##Trigger", &trigger))
    {
        SetTrigger(trigger);
    }

    ImGui::Text("Center");
    glm::vec3 center = this->center;
    if (ImGui::InputFloat3("##Center", &center.x))
    {
        SetCenter(center);
    }

    ImGui::Text("Static Friction");
    float sF = staticFriction;
    if (ImGui::InputFloat("##Static Friction", &sF))
    {
        SetStaticFriction(sF);
    }

    ImGui::Text("Dynamic Friction");
    float dF = dynamicFriction;
    if (ImGui::InputFloat("##Dynamic Friction", &dF))
    {
        SetDynamicFriction(dF);
    }

    ImGui::Text("Restitution");
    float restitution = this->restitution;
    if (ImGui::InputFloat("##Restitution", &restitution))
    {
        SetRestitution(restitution);
    }
}

void Collider::SetCenter(glm::vec3 center)
{
    this->center = center;
    if (attachedRigidbody) attachedRigidbody->UpdateShapeProperties(this);
}

void Collider::SetTrigger(bool trigger)
{
    isTrigger = trigger;
    if (attachedRigidbody) attachedRigidbody->UpdateShapeProperties(this);
}

void Collider::SetStaticFriction(float staticFriction)
{
    this->staticFriction = glm::clamp(staticFriction, 0.0f, INFINITY);
    if (attachedRigidbody) attachedRigidbody->UpdateShapeProperties(this);
}

void Collider::SetDynamicFriction(float dynamicFriction)
{
    this->dynamicFriction = glm::clamp(dynamicFriction, 0.0f, INFINITY);
    if (attachedRigidbody) attachedRigidbody->UpdateShapeProperties(this);
}
void Collider::SetRestitution(float restitution)
{
    this->restitution = glm::clamp(restitution, 0.0f, INFINITY);
    if (attachedRigidbody) attachedRigidbody->UpdateShapeProperties(this);
}

void Collider::OnGameObjectEvent(GameObjectEvent event, Component* component) 
{
    switch (event)
    {
    case GameObjectEvent::HIERARCHY_CHANGED:
        
        Rigidbody* newRB = (Rigidbody*)owner->GetComponentInParent(ComponentType::RIGIDBODY);

        if (newRB != attachedRigidbody)
        {
            if (attachedRigidbody) {
                attachedRigidbody->CreateBody();
            }

            attachedRigidbody = newRB;

            if (attachedRigidbody) {
                attachedRigidbody->CreateBody();
            }
            else {
               
            }
        }
        else if (attachedRigidbody)
        {
            attachedRigidbody->CreateBody();
        }
        break;
    }
}
