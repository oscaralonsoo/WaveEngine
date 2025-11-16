#include "ComponentCamera.h"
#include "GameObject.h"
#include "Transform.h"
#include <glm/gtc/quaternion.hpp>
#include <iostream>


using namespace std;

ComponentCamera::ComponentCamera(GameObject* owner)
    : Component(owner, ComponentType::CAMERA),
    viewMatrix(1.0f),
    projectionMatrix(1.0f),
    fov(45.0f),
    aspectRatio(16.0f / 9.0f),
    nearPlane(0.1f),
    farPlane(100.0f),
    yaw(-90.0f),
    pitch(0.0f),
    lastX(400.0f),
    lastY(300.0f),
    firstMouse(true),
    orbitTarget(0.0f, 0.0f, 0.0f),
    orbitDistance(10.0f),
    lastOrbitX(400.0f),
    lastOrbitY(300.0f),
    firstOrbit(true),
    lastPanX(400.0f),
    lastPanY(300.0f),
    firstPan(true),
    mouseSensitivity(0.2f),
    scrollSpeed(0.5f),
    panSensitivity(0.003f),
    movementSpeed(2.5f),
    cameraFront(0.0f, 0.0f, -1.0f),
    cameraUp(0.0f, 1.0f, 0.0f),
    frustumCullingEnabled(false),
    drawFrustum(false)
{
    name = "Camera";
    UpdateProjectionMatrix();
    UpdateFrustum();
}

ComponentCamera::~ComponentCamera()
{
}

void ComponentCamera::Update()
{
    UpdateViewMatrix();
    UpdateFrustum();
}

void ComponentCamera::OnEditor()
{
}

void ComponentCamera::SetAspectRatio(float newAspectRatio)
{
    aspectRatio = newAspectRatio;
    UpdateProjectionMatrix();
    UpdateFrustum();
}

void ComponentCamera::UpdateViewMatrix()
{
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        glm::mat4 globalMatrix = transform->GetGlobalMatrix();
        glm::vec3 position = glm::vec3(globalMatrix[3]);
        // En OpenGL, forward es -Z local
        glm::vec3 forward = -glm::normalize(glm::vec3(globalMatrix[2]));
        glm::vec3 up = glm::normalize(glm::vec3(globalMatrix[1]));

        viewMatrix = glm::lookAt(position, position + forward, up);
    }
}

void ComponentCamera::UpdateProjectionMatrix()
{
    projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void ComponentCamera::UpdateFrustum()
{
    glm::mat4 viewProj = projectionMatrix * viewMatrix;
    frustum.ExtractFromMatrix(viewProj);
}

glm::vec3 ComponentCamera::GetPosition() const
{
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        glm::mat4 globalMatrix = transform->GetGlobalMatrix();
        return glm::vec3(globalMatrix[3]);
    }
    return glm::vec3(0.0f);
}

glm::vec3 ComponentCamera::GetFront() const
{
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        glm::mat4 globalMatrix = transform->GetGlobalMatrix();
        return -glm::normalize(glm::vec3(globalMatrix[2]));
    }
    return glm::vec3(0.0f, 0.0f, -1.0f);
}

glm::vec3 ComponentCamera::GetUp() const
{
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        glm::mat4 globalMatrix = transform->GetGlobalMatrix();
        return glm::normalize(glm::vec3(globalMatrix[1]));
    }
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

void ComponentCamera::UpdateCameraVectors()
{
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(direction);

    // Update the Transform
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        glm::vec3 right = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
        cameraUp = glm::normalize(glm::cross(right, cameraFront));

        glm::mat4 rotationMatrix(1.0f);
        rotationMatrix[0] = glm::vec4(right, 0.0f);
        rotationMatrix[1] = glm::vec4(cameraUp, 0.0f);
        rotationMatrix[2] = glm::vec4(-cameraFront, 0.0f);

        glm::quat rotation = glm::quat_cast(rotationMatrix);
        transform->SetRotationQuat(rotation);
    }
}

void ComponentCamera::ProcessMouseMovement(float xoffset, float yoffset)
{
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    UpdateCameraVectors();
}

void ComponentCamera::HandleMouseInput(float xpos, float ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    ProcessMouseMovement(xoffset, yoffset);
}

void ComponentCamera::HandleScrollInput(float yoffset)
{
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        glm::vec3 position = GetPosition();
        glm::vec3 front = GetFront();

        position += front * yoffset * scrollSpeed;
        transform->SetPosition(position);

        orbitDistance = glm::length(position - orbitTarget);
    }
}

void ComponentCamera::HandleOrbitInput(float xpos, float ypos)
{
    if (firstOrbit)
    {
        lastOrbitX = xpos;
        lastOrbitY = ypos;
        firstOrbit = false;
        glm::vec3 position = GetPosition();
        glm::vec3 front = GetFront();
        orbitTarget = position + front * orbitDistance;
        orbitDistance = glm::length(position - orbitTarget);
        return;
    }

    float xoffset = xpos - lastOrbitX;
    float yoffset = lastOrbitY - ypos;
    lastOrbitX = xpos;
    lastOrbitY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    UpdateOrbitPosition();
}

void ComponentCamera::UpdateOrbitPosition()
{
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(direction);

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        glm::vec3 newPosition = orbitTarget - cameraFront * orbitDistance;
        transform->SetPosition(newPosition);

        glm::vec3 right = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
        cameraUp = glm::normalize(glm::cross(right, cameraFront));

        glm::mat4 rotationMatrix(1.0f);
        rotationMatrix[0] = glm::vec4(right, 0.0f);
        rotationMatrix[1] = glm::vec4(cameraUp, 0.0f);
        rotationMatrix[2] = glm::vec4(-cameraFront, 0.0f);

        glm::quat rotation = glm::quat_cast(rotationMatrix);
        transform->SetRotationQuat(rotation);
    }
}

void ComponentCamera::HandlePanInput(float xoffset, float yoffset)
{
    glm::vec3 front = GetFront();
    glm::vec3 up = GetUp();
    glm::vec3 right = glm::normalize(glm::cross(front, up));
    up = glm::normalize(glm::cross(right, front));

    glm::vec3 offset = right * (-xoffset * panSensitivity * orbitDistance) +
        up * (yoffset * panSensitivity * orbitDistance);

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        glm::vec3 position = GetPosition();
        transform->SetPosition(position + offset);
    }

    orbitTarget += offset;
}

void ComponentCamera::FocusOnTarget(const glm::vec3& targetPosition, float targetRadius)
{
    orbitTarget = targetPosition;
    orbitDistance = targetRadius * 0.05f;

    if (orbitDistance < 2.0f)
        orbitDistance = 2.0f;

    if (orbitDistance > 30.0f)
        orbitDistance = 30.0f;

    UpdateCameraVectors();

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        glm::vec3 newPosition = orbitTarget - cameraFront * orbitDistance;
        transform->SetPosition(newPosition);
    }
}

glm::vec3 ComponentCamera::ScreenToWorldRay(int mouseX, int mouseY, int screenWidth, int screenHeight) const
{
    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;

    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projectionMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::vec4 rayWorld = glm::inverse(viewMatrix) * rayEye;
    glm::vec3 rayDir = glm::normalize(glm::vec3(rayWorld));

    return rayDir;
}

void ComponentCamera::GetFrustumCorners(glm::vec3 corners[8]) const
{
    // Get camera properties
    glm::vec3 position = GetPosition();
    glm::vec3 front = GetFront();
    glm::vec3 up = GetUp();
    glm::vec3 right = glm::normalize(glm::cross(front, up));

    // Calculate near and far plane centers
    glm::vec3 nearCenter = position + front * nearPlane;
    glm::vec3 farCenter = position + front * farPlane;

    // Calculate near and far plane dimensions
    float nearHeight = 2.0f * tan(glm::radians(fov) / 2.0f) * nearPlane;
    float nearWidth = nearHeight * aspectRatio;
    float farHeight = 2.0f * tan(glm::radians(fov) / 2.0f) * farPlane;
    float farWidth = farHeight * aspectRatio;

    // Near plane corners (0-3)
    corners[0] = nearCenter - right * (nearWidth / 2.0f) - up * (nearHeight / 2.0f); // Bottom-left
    corners[1] = nearCenter + right * (nearWidth / 2.0f) - up * (nearHeight / 2.0f); // Bottom-right
    corners[2] = nearCenter + right * (nearWidth / 2.0f) + up * (nearHeight / 2.0f); // Top-right
    corners[3] = nearCenter - right * (nearWidth / 2.0f) + up * (nearHeight / 2.0f); // Top-left

    // Far plane corners (4-7)
    corners[4] = farCenter - right * (farWidth / 2.0f) - up * (farHeight / 2.0f); // Bottom-left
    corners[5] = farCenter + right * (farWidth / 2.0f) - up * (farHeight / 2.0f); // Bottom-right
    corners[6] = farCenter + right * (farWidth / 2.0f) + up * (farHeight / 2.0f); // Top-right
    corners[7] = farCenter - right * (farWidth / 2.0f) + up * (farHeight / 2.0f); // Top-left
}