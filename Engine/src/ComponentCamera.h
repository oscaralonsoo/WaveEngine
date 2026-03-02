#pragma once
#include "Component.h"
#include <glm/glm.hpp>

class CameraLens;

class ComponentCamera : public Component
{
public:
    ComponentCamera(GameObject* owner);
    ~ComponentCamera();

    void Update() override;

    bool IsType(ComponentType type) override { return type == ComponentType::CAMERA; };
    bool IsIncompatible(ComponentType type) override { return type == ComponentType::CAMERA; };

    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    bool IsMainCamera() const;
    void SetMainCamera(bool b);

    CameraLens* GetLens() { return lens; }

    const glm::mat4& GetViewMatrix() const;
    const glm::mat4& GetProjectionMatrix() const;

    glm::vec3 ScreenToWorldRay(int mouseX, int mouseY, int screenWidth, int screenHeight) const;

private:
    void SyncTransformToLens();

    CameraLens* lens = nullptr;

};