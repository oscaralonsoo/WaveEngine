#pragma once
#include <glm/glm.hpp>
#include "glm/gtc/quaternion.hpp"

class Camera
{
public:
	Camera();
	~Camera();

	void Update();

	const glm::mat4& GetViewMatrix() const { return viewMatrix; }
	const glm::mat4& GetProjectionMatrix() const { return projectionMatrix; }

private:
	glm::vec3 position;
	glm::vec3 target;
	glm::vec3 up;

	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
};