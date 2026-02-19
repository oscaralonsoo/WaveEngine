#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform : public Component {
public:
    Transform(GameObject* owner);
    ~Transform() = default;

    void Update() override;
    void OnEditor() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    const glm::vec3& GetPosition() const { return position; }
    const glm::vec3& GetRotation() const { return rotation; }
    const glm::vec3& GetScale() const { return scale; }
    const glm::quat& GetRotationQuat() const { return rotationQuat; }

    void SetPosition(const glm::vec3& pos);
    void SetRotation(const glm::vec3& rot);
    void SetRotationQuat(const glm::quat& quat);
    void SetScale(const glm::vec3& scl);

    const glm::mat4& GetLocalMatrix();
    const glm::mat4& GetGlobalMatrix();

    //for audio coords
    glm::mat4 GetWorldMatrixRecursive();

    void UpdateLocalMatrix();
    void UpdateGlobalMatrix();


    // Getter para el GameObject propietario (necesario para validaciones en Lua)
    GameObject* GetOwner() const { return owner; }

    glm::vec3 GetForward() const { return rotationQuat * glm::vec3(0.0f, 0.0f, 1.0f); }
    glm::vec3 GetUp() const { return rotationQuat * glm::vec3(0.0f, 1.0f, 0.0f); }


private:
    // Transforms
    glm::vec3 position;
    glm::vec3 rotation;    // Euler angles
    glm::quat rotationQuat; // Quaternions
    glm::vec3 scale;

    // Matrixs
    glm::mat4 localMatrix;
    glm::mat4 globalMatrix;

    // Dirty flags
    bool localDirty = true;
    bool globalDirty = true;

    void UpdateQuaternionFromEuler();
    void UpdateEulerFromQuaternion();
    void MarkChildrenGlobalDirty();
};