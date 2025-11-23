#include "Transform.h"
#include "GameObject.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "Application.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

Transform::Transform(GameObject* owner)
    : Component(owner, ComponentType::TRANSFORM),
    position(0.0f, 0.0f, 0.0f),
    rotation(0.0f, 0.0f, 0.0f),
    rotationQuat(1.0f, 0.0f, 0.0f, 0.0f), 
    scale(1.0f, 1.0f, 1.0f),
    localMatrix(1.0f),
    globalMatrix(1.0f),
    localDirty(true),
    globalDirty(true)
{
}

void Transform::Update()
{

}

void Transform::OnEditor()
{

}

void Transform::SetPosition(const glm::vec3& pos)
{
    if (position != pos)
    {
        position = pos;
        localDirty = true;
        globalDirty = true;
        MarkChildrenGlobalDirty();

        UpdateOctree();
    }
}

void Transform::SetRotation(const glm::vec3& rot)
{
    if (rotation != rot)
    {
        rotation = rot;
        UpdateQuaternionFromEuler();
        localDirty = true;
        globalDirty = true;
        MarkChildrenGlobalDirty();

        UpdateOctree();
    }
}

void Transform::SetRotationQuat(const glm::quat& quat)
{
    if (rotationQuat != quat)
    {
        rotationQuat = quat;
        UpdateEulerFromQuaternion();
        localDirty = true;
        globalDirty = true;
        MarkChildrenGlobalDirty();

        UpdateOctree();
    }
}

void Transform::SetScale(const glm::vec3& scl)
{
    if (scale != scl)
    {
        scale = scl;
        localDirty = true;
        globalDirty = true;
        MarkChildrenGlobalDirty();

        UpdateOctree();
    }
}

const glm::mat4& Transform::GetLocalMatrix()
{
    if (localDirty)
    {
        UpdateLocalMatrix();
    }
    return localMatrix;
}

const glm::mat4& Transform::GetGlobalMatrix()
{
    if (globalDirty)
    {
        UpdateGlobalMatrix();
    }
    return globalMatrix;
}

void Transform::UpdateLocalMatrix()
{
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotationMatrix = glm::mat4_cast(rotationQuat); 
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

    localMatrix = translationMatrix * rotationMatrix * scaleMatrix;
    localDirty = false;
}

void Transform::UpdateGlobalMatrix()
{
    if (localDirty)
    {
        UpdateLocalMatrix();
    }

    GameObject* parent = owner->GetParent();

    if (parent != nullptr)
    {
        Transform* parentTransform = static_cast<Transform*>(parent->GetComponent(ComponentType::TRANSFORM));

        if (parentTransform != nullptr)
        {
            globalMatrix = parentTransform->GetGlobalMatrix() * localMatrix;
        }
        else
        {
            globalMatrix = localMatrix;
        }
    }
    else
    {
        globalMatrix = localMatrix;
    }

    globalDirty = false;
}

void Transform::MarkChildrenGlobalDirty()
{
    const std::vector<GameObject*>& children = owner->GetChildren();

    for (GameObject* child : children)
    {
        Transform* childTransform = static_cast<Transform*>(child->GetComponent(ComponentType::TRANSFORM));

        if (childTransform != nullptr)
        {
            childTransform->globalDirty = true;
            childTransform->MarkChildrenGlobalDirty();
        }
    }
}

void Transform::UpdateQuaternionFromEuler()
{
    glm::vec3 radians = glm::radians(rotation);
    rotationQuat = glm::quat(radians);
}

void Transform::UpdateEulerFromQuaternion()
{
    rotation = glm::degrees(glm::eulerAngles(rotationQuat));
}

void Transform::UpdateOctree()
{
    Application& app = Application::GetInstance();
    if (app.scene && owner)
    {
        app.scene->UpdateObjectInOctree(owner);
    }
}