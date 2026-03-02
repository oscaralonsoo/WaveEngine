#pragma once

#include "EditorCommand.h"
#include <memory>

class GameObject;

class CreateCommand : public EditorCommand
{
public:
    CreateCommand(GameObject* object);

    void Execute() override;
    void Undo() override;

private:
    GameObject* m_Object = nullptr;
    std::unique_ptr<GameObject> m_OwnedObject;

    GameObject* m_Parent = nullptr;
    int m_ChildIndex = -1;
};