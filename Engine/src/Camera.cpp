#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3/SDL.h>

Camera::Camera()
	: position(0.0f, 2.0f, 0.0f),
	target(0.0f, 0.0f, 0.0f),
	up(0.0f, 1.0f, 0.0f)
{
}

Camera::~Camera()
{
}

void Camera::Update()
{
	// Crear matriz de proyección (perspectiva)
	projectionMatrix = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

	// Crear matriz de vista con cámara orbitando
	const float radius = 10.0f;
	float time = SDL_GetTicks() / 1000.0f;
	float camX = sin(time) * radius;
	float camZ = cos(time) * radius;

	position = glm::vec3(camX, 2.0f, camZ);

	viewMatrix = glm::lookAt(
		position,
		target,
		up
	);
}