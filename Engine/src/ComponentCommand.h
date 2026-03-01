#pragma once

#include "EditorCommand.h"
#include "GameObject.h"
#include "Application.h"
#include "ModuleEditor.h"
#include <memory>

class RemoveComponentCommand : public EditorCommand
{
public:
    RemoveComponentCommand(GameObject* object, Component* component)
        : m_Object(object)
        , m_Component(component)
        , m_Index(object->GetComponentIndex(component))
    {}

    void Execute() override
    {
        if (!m_Object || !m_Component) return;
        m_Owned = m_Object->ExtractComponent(m_Component);
    }

    void Undo() override
    {
        if (!m_Object || !m_Owned) return;
        m_Object->ReinsertComponentAt(std::move(m_Owned), m_Index);
        m_Component = m_Object->GetComponents()[m_Index];
    }

private:
    GameObject* m_Object;
    Component* m_Component;
    int m_Index;
    std::unique_ptr<Component> m_Owned;
};

class AddComponentCommand : public EditorCommand
{
public:
    AddComponentCommand(GameObject* object, Component* component)
        : m_Object(object)
        , m_Component(component)
        , m_Index(object->GetComponentIndex(component))
    {}

    void Undo() override
    {
        if (!m_Object || !m_Component) return;
        m_Owned = m_Object->ExtractComponent(m_Component);
    }

    void Execute() override
    {
        if (!m_Object || !m_Owned) return;
        m_Object->ReinsertComponentAt(std::move(m_Owned), m_Index);
        m_Component = m_Object->GetComponents()[m_Index];
    }

private:
    GameObject* m_Object;
    Component* m_Component;
    int m_Index;
    std::unique_ptr<Component> m_Owned;
};
