#pragma once

#include "EditorCommand.h"
#include <glm/glm.hpp>

class GameObject;

class TransformCommand : public EditorCommand
{
public:
    TransformCommand(GameObject* object,
        const glm::vec3& oldPos, const glm::vec3& oldRot, const glm::vec3& oldScale,
        const glm::vec3& newPos, const glm::vec3& newRot, const glm::vec3& newScale);

    void Execute() override;
    void Undo() override;

private:
    void ApplyTransform(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scl);

    GameObject* m_Object;

    glm::vec3 m_OldPosition;
    glm::vec3 m_OldRotation;
    glm::vec3 m_OldScale;

    glm::vec3 m_NewPosition;
    glm::vec3 m_NewRotation;
    glm::vec3 m_NewScale;
};
