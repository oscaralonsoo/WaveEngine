#include "Transform.h"
#include "GameObject.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>
#include "Log.h"

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
    // Transform doesn't need per-frame updates
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

void Transform::Serialize(nlohmann::json& componentObj) const
{
    // Position
    componentObj["position"] = { position.x, position.y, position.z };

    // Rotation
    componentObj["rotation"] = { rotation.x, rotation.y, rotation.z };

    // Scale
    componentObj["scale"] = { scale.x, scale.y, scale.z };
}

void Transform::Deserialize(const nlohmann::json& componentObj)
{
    // Position
    if (componentObj.contains("position") && componentObj["position"].is_array()) {
        auto& posArray = componentObj["position"];
        if (posArray.size() >= 3) {
            position = glm::vec3(
                posArray[0].get<float>(),
                posArray[1].get<float>(),
                posArray[2].get<float>()
            );
            localDirty = true;
            globalDirty = true;
        }
    }

    // Rotation
    if (componentObj.contains("rotation") && componentObj["rotation"].is_array()) {
        auto& rotArray = componentObj["rotation"];
        if (rotArray.size() >= 3) {
            rotation = glm::vec3(
                rotArray[0].get<float>(),
                rotArray[1].get<float>(),
                rotArray[2].get<float>()
            );
            UpdateQuaternionFromEuler();
            localDirty = true;
            globalDirty = true;
        }
    }

    // Scale
    if (componentObj.contains("scale") && componentObj["scale"].is_array()) {
        auto& scaleArray = componentObj["scale"];
        if (scaleArray.size() >= 3) {
            scale = glm::vec3(
                scaleArray[0].get<float>(),
                scaleArray[1].get<float>(),
                scaleArray[2].get<float>()
            );
            localDirty = true;
            globalDirty = true;
        }
    }
}