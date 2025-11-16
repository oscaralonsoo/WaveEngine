#pragma once
#include "Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class ComponentCamera : public Component
{
public:
    ComponentCamera(GameObject* owner);
    ~ComponentCamera();

    void Update() override;
    void OnEditor() override;

    // View & Projection
    const glm::mat4& GetViewMatrix() const { return viewMatrix; }
    const glm::mat4& GetProjectionMatrix() const { return projectionMatrix; }

    // Getters
    glm::vec3 GetPosition() const;
    glm::vec3 GetFront() const;
    glm::vec3 GetUp() const;
    float GetFov() const { return fov; }
    float GetAspectRatio() const { return aspectRatio; }
    float GetNearPlane() const { return nearPlane; }
    float GetFarPlane() const { return farPlane; }

    // Setters
    void SetAspectRatio(float aspectRatio);
    void SetFov(float newFov) { 
        fov = newFov; 
        UpdateProjectionMatrix(); 
    }
    void SetNearPlane(float nearDist) {
        nearPlane = nearDist;
        UpdateProjectionMatrix(); 
    }
    void SetFarPlane(float farDist) {
        farPlane = farDist;
        UpdateProjectionMatrix(); 
    }

    // Editor Controls
    void HandleMouseInput(float xpos, float ypos);
    void HandleScrollInput(float yoffset);
    void HandleOrbitInput(float xpos, float ypos);
    void HandlePanInput(float xoffset, float yoffset);
    void ResetMouseInput() { firstMouse = true; }
    void ResetOrbitInput() { firstOrbit = true; }
    void ResetPanInput() { firstPan = true; }

    // Camera utilities
    void FocusOnTarget(const glm::vec3& targetPosition, float targetRadius = 1.0f);
    void SetOrbitTarget(const glm::vec3& target) { orbitTarget = target; }
    glm::vec3 GetOrbitTarget() const { return orbitTarget; }
    glm::vec3 ScreenToWorldRay(int mouseX, int mouseY, int screenWidth, int screenHeight) const;

    // Configuration
    float GetMouseSensitivity() const { return mouseSensitivity; }
    void SetMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
    float GetScrollSpeed() const { return scrollSpeed; }
    void SetScrollSpeed(float speed) { scrollSpeed = speed; }
    float GetPanSensitivity() const { return panSensitivity; }
    void SetPanSensitivity(float sensitivity) { panSensitivity = sensitivity; }
    float GetMovementSpeed() const { return movementSpeed; }
    void SetMovementSpeed(float speed) { movementSpeed = speed; }

private:
    // Matrices
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

    // Camera parameters
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;

    // Camera orientation
    float yaw;
    float pitch;

    // Mouse control
    float lastX;
    float lastY;
    bool firstMouse;

    // Orbit control
    glm::vec3 orbitTarget;
    float orbitDistance;
    float lastOrbitX;
    float lastOrbitY;
    bool firstOrbit;

    // Pan control
    float lastPanX;
    float lastPanY;
    bool firstPan;

    // Settings
    float mouseSensitivity;
    float scrollSpeed;
    float panSensitivity;
    float movementSpeed;

    // Updates
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();
    void UpdateCameraVectors();
    void ProcessMouseMovement(float xoffset, float yoffset);
    void UpdateOrbitPosition();

    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
};