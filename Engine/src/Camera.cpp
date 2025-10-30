#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL.h>

Camera::Camera()
	: cameraPos(0.0f, 5.0f, 10.0f),
	cameraFront(0.0f, 0.0f, -1.0f),
	cameraUp(0.0f, 1.0f, 0.0f),
	viewMatrix(1.0f),
	projectionMatrix(1.0f),
	yaw(-90.0f),
	pitch(0.0f),
	lastX(400.0f),
	lastY(300.0f),
	firstMouse(true),
	fov(45.0f),
	aspectRatio(16.0f / 9.0f),
	orbitTarget(0.0f, 0.0f, 0.0f),
	orbitDistance(3.0f),
	lastOrbitX(400.0f),
	lastOrbitY(300.0f),
	firstOrbit(true),
	lastPanX(400.0f),
	lastPanY(300.0f),
	firstPan(true)
{
	UpdateProjectionMatrix();
}

Camera::~Camera()
{
}

void Camera::UpdateCameraVectors()
{
	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	cameraFront = glm::normalize(direction);
}

void Camera::UpdateProjectionMatrix()
{
	projectionMatrix = glm::perspective(
		glm::radians(fov),
		aspectRatio,
		0.1f,
		100.0f
	);
}

void Camera::SetAspectRatio(float newAspectRatio)
{
	if (aspectRatio != newAspectRatio)
	{
		aspectRatio = newAspectRatio;
		UpdateProjectionMatrix();
	}
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset)
{
	const float sensitivity = 0.2f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	UpdateCameraVectors();
}

void Camera::HandleMouseInput(float xpos, float ypos)
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

void Camera::HandleScrollInput(float yoffset)
{
	float zoomSpeed = 0.5f;
	cameraPos += cameraFront * yoffset * zoomSpeed;

	orbitDistance = glm::length(cameraPos - orbitTarget);
}

void Camera::HandleOrbitInput(float xpos, float ypos)
{
	if (firstOrbit)
	{
		lastOrbitX = xpos;
		lastOrbitY = ypos;
		firstOrbit = false;
		orbitTarget = cameraPos + cameraFront * orbitDistance;
		// Calculate the current distance to the target
		orbitDistance = glm::length(cameraPos - orbitTarget);
		return;
	}

	float xoffset = xpos - lastOrbitX;
	float yoffset = lastOrbitY - ypos;
	lastOrbitX = xpos;
	lastOrbitY = ypos;

	const float sensitivity = 0.2f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// Limit pitch to avoid gimbal lock
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	UpdateOrbitPosition();
}

void Camera::UpdateOrbitPosition()
{
	// Calculate the new position of the camera in orbit around the target
	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	cameraFront = glm::normalize(direction);
	cameraPos = orbitTarget - cameraFront * orbitDistance;
}

void Camera::HandlePanInput(float xoffset, float yoffset)
{
	const float panSensitivity = 0.003f;

	// Calculate the right and up vectors of the camera
	glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
	glm::vec3 up = glm::normalize(glm::cross(right, cameraFront));

	// Move the camera and the target
	glm::vec3 offset = right * (-xoffset * panSensitivity * orbitDistance) +
		up * (yoffset * panSensitivity * orbitDistance);

	cameraPos += offset;
	orbitTarget += offset;
}

void Camera::FocusOnTarget(const glm::vec3& targetPosition, float targetRadius)
{
	orbitTarget = targetPosition;

	// Calculate an appropriate distance based on the object's radius
	// Multiplicador balanceado para buena visualización
	orbitDistance = targetRadius * 0.05;

	if (orbitDistance < 2.0f)
		orbitDistance = 2.0f;

	if (orbitDistance > 30.0f)
		orbitDistance = 30.0f;

	// Position the camera facing the target from the current direction
	UpdateCameraVectors();
	cameraPos = orbitTarget - cameraFront * orbitDistance;
}

void Camera::Update()
{
	UpdateCameraVectors();
	viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

glm::vec3 Camera::ScreenToWorldRay(int mouseX, int mouseY, int screenWidth, int screenHeight) const
{
	// Convert screen coordinates to Normalized Device Coordinates (NDC)
	float x = (2.0f * mouseX) / screenWidth - 1.0f;
	float y = 1.0f - (2.0f * mouseY) / screenHeight;  // Invert Y axis because screen Y goes down

	// Create a point in clip space (homogeneous coordinates)
	glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);

	// Convert from clip space to eye (camera) space
	glm::mat4 projMatrix = GetProjectionMatrix();
	glm::vec4 rayEye = glm::inverse(projMatrix) * rayClip;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f); // Set forward direction in eye space

	// Convert from eye space to world space
	glm::mat4 viewMatrix = GetViewMatrix();
	glm::vec4 rayWorld = glm::inverse(viewMatrix) * rayEye;

	// Normalize the direction vector
	glm::vec3 rayDir = glm::normalize(glm::vec3(rayWorld));

	return rayDir;
}