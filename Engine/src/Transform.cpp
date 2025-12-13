#include "Transform.h"
#include "GameObject.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <rapidjson/document.h>
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

void Transform::Serialize(rapidjson::Value& componentObj, rapidjson::Value::AllocatorType& allocator) const
{
    // Position
    rapidjson::Value posArray(rapidjson::kArrayType);
    posArray.PushBack(position.x, allocator);
    posArray.PushBack(position.y, allocator);
    posArray.PushBack(position.z, allocator);
    componentObj.AddMember("position", posArray, allocator);

    // Rotation
    rapidjson::Value rotArray(rapidjson::kArrayType);
    rotArray.PushBack(rotation.x, allocator);
    rotArray.PushBack(rotation.y, allocator);
    rotArray.PushBack(rotation.z, allocator);
    componentObj.AddMember("rotation", rotArray, allocator);

    // Scale
    rapidjson::Value scaleArray(rapidjson::kArrayType);
    scaleArray.PushBack(scale.x, allocator);
    scaleArray.PushBack(scale.y, allocator);
    scaleArray.PushBack(scale.z, allocator);
    componentObj.AddMember("scale", scaleArray, allocator);
}

void Transform::Deserialize(const rapidjson::Value& componentObj)
{
    // Position
    if (componentObj.HasMember("position") && componentObj["position"].IsArray()) {
        const rapidjson::Value& posArray = componentObj["position"];
        if (posArray.Size() >= 3) {
            position = glm::vec3(
                posArray[0].GetFloat(),
                posArray[1].GetFloat(),
                posArray[2].GetFloat()
            );
            localDirty = true;
            globalDirty = true;
        }
    }

    // Rotation
    if (componentObj.HasMember("rotation") && componentObj["rotation"].IsArray()) {
        const rapidjson::Value& rotArray = componentObj["rotation"];
        if (rotArray.Size() >= 3) {
            rotation = glm::vec3(
                rotArray[0].GetFloat(),
                rotArray[1].GetFloat(),
                rotArray[2].GetFloat()
            );
            UpdateQuaternionFromEuler();
            localDirty = true;
            globalDirty = true;
        }
    }

    // Scale
    if (componentObj.HasMember("scale") && componentObj["scale"].IsArray()) {
        const rapidjson::Value& scaleArray = componentObj["scale"];
        if (scaleArray.Size() >= 3) {
            scale = glm::vec3(
                scaleArray[0].GetFloat(),
                scaleArray[1].GetFloat(),
                scaleArray[2].GetFloat()
            );
            localDirty = true;
            globalDirty = true;
        }
    }
}