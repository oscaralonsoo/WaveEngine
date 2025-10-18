#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL.h>

Camera::Camera()
	: cameraPos(0.0f, 0.0f, 3.0f),
	cameraFront(0.0f, 0.0f, -1.0f),
	cameraUp(0.0f, 1.0f, 0.0f)
{
}

Camera::~Camera()
{
}

void Camera::Update()
{
	// Crear matriz de proyección (perspectiva)
	projectionMatrix = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

	// Crear matriz de vista
	viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}
