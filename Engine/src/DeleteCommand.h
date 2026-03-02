#pragma once

#include "EditorCommand.h"
#include <memory>
#include <string>

class GameObject;

class DeleteCommand : public EditorCommand
{
public:
    DeleteCommand(GameObject* object);

    void Execute() override;
    void Undo() override;

private:
    GameObject* m_Object = nullptr;
    std::unique_ptr<GameObject>  m_OwnedObject;

    GameObject* m_Parent = nullptr;
    int m_ChildIndex = -1;
};