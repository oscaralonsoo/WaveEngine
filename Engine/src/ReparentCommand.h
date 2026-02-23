#pragma once

#include "EditorCommand.h"
#include "GameObject.h"
#include "Application.h"
#include "ModuleScene.h"

class ReparentCommand : public EditorCommand
{
public:
    ReparentCommand(GameObject* object, GameObject* newParent, int newIndex)
        : m_Object(object)
        , m_OldParent(object->GetParent())
        , m_OldIndex(m_OldParent ? m_OldParent->GetChildIndex(object) : -1)
        , m_NewParent(newParent)
        , m_NewIndex(newIndex)
    {}

    void Execute() override { Apply(m_NewParent, m_NewIndex); }
    void Undo() override { Apply(m_OldParent, m_OldIndex); }

private:
    void Apply(GameObject* parent, int index)
    {
        if (!m_Object || !parent) return;
        parent->InsertChildAt(m_Object, index);
        Application::GetInstance().scene->MarkOctreeForRebuild();
    }

    GameObject* m_Object;
    GameObject* m_OldParent;
    int m_OldIndex;
    GameObject* m_NewParent;
    int m_NewIndex;
};
