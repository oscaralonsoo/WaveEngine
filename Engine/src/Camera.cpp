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
	fov -= yoffset;

	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 45.0f)
		fov = 45.0f;

	UpdateProjectionMatrix();  
}

void Camera::HandleOrbitInput(float xpos, float ypos)
{
	if (firstOrbit)
	{
		lastOrbitX = xpos;
		lastOrbitY = ypos;
		firstOrbit = false;
		// Calculate the current distance to the target
		orbitDistance = glm::length(cameraPos - orbitTarget);
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
	const float panSensitivity = 0.005f;

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
	orbitDistance = targetRadius * 2.5f;
	if (orbitDistance < 1.0f)
		orbitDistance = 3.0f;

	// Position the camera facing the target from the current direction
	UpdateCameraVectors();
	cameraPos = orbitTarget - cameraFront * orbitDistance;
}

void Camera::Update()
{
	UpdateCameraVectors();
	viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}