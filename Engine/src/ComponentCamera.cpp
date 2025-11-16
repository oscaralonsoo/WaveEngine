#include "ComponentCamera.h"
#include "GameObject.h"
#include "Transform.h"
#include <glm/gtc/quaternion.hpp>

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
    cameraUp(0.0f, 1.0f, 0.0f)
{
    name = "Camera";
    UpdateProjectionMatrix();
}

ComponentCamera::~ComponentCamera()
{
}

void ComponentCamera::Update()
{
    UpdateViewMatrix();
}

void ComponentCamera::OnEditor()
{
}

void ComponentCamera::SetAspectRatio(float newAspectRatio)
{
    aspectRatio = newAspectRatio;
    UpdateProjectionMatrix();
}

void ComponentCamera::UpdateViewMatrix()
{
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        glm::mat4 globalMatrix = transform->GetGlobalMatrix();
        glm::vec3 position = glm::vec3(globalMatrix[3]);
        glm::vec3 forward = glm::normalize(glm::vec3(globalMatrix[2]));
        glm::vec3 up = glm::normalize(glm::vec3(globalMatrix[1]));

        viewMatrix = glm::lookAt(position, position - forward, up);
    }
}

void ComponentCamera::UpdateProjectionMatrix()
{
    projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
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
        return glm::normalize(glm::vec3(globalMatrix[2]));
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
        // Calculate the current distance to the target
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

    // Limit pitch to avoid gimbal lock
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    UpdateOrbitPosition();
}

void ComponentCamera::UpdateOrbitPosition()
{
    // Calculate the new position of the camera in orbit around the target
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

        // Update rotation
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
    // Calculate the right and up vectors of the camera
    glm::vec3 front = GetFront();
    glm::vec3 up = GetUp();
    glm::vec3 right = glm::normalize(glm::cross(front, up));
    up = glm::normalize(glm::cross(right, front));

    // Move the camera and the target
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

    // Calculate an appropriate distance based on the object's radius
    // Multiplicador balanceado para buena visualización
    orbitDistance = targetRadius * 0.05f;

    if (orbitDistance < 2.0f)
        orbitDistance = 2.0f;

    if (orbitDistance > 30.0f)
        orbitDistance = 30.0f;

    // Position the camera facing the target from the current direction
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
    // Convert screen coordinates to Normalized Device Coordinates (NDC)
    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;

    // Create a point in clip space (homogeneous coordinates)
    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);

    // Convert from clip space to eye (camera) space
    glm::vec4 rayEye = glm::inverse(projectionMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f); // Set forward direction in eye space

    // Convert from eye space to world space
    glm::vec4 rayWorld = glm::inverse(viewMatrix) * rayEye;

    // Normalize the direction vector
    glm::vec3 rayDir = glm::normalize(glm::vec3(rayWorld));

    return rayDir;
}