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

    void UpdateLocalMatrix();
    void UpdateGlobalMatrix();

    void SetGlobalPosition(const glm::vec3& globalPos);
    void SetGlobalRotationQuat(const glm::quat& globalRot);

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