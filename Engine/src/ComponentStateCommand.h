#pragma once

#include "EditorCommand.h"
#include "Component.h"
#include <nlohmann/json.hpp>

class ComponentStateCommand : public EditorCommand
{
public:
    ComponentStateCommand(Component* comp, nlohmann::json before, nlohmann::json after)
        : m_Comp(comp)
        , m_Before(std::move(before))
        , m_After(std::move(after))
    {}

    void Execute() override { m_Comp->Deserialize(m_After);  }
    void Undo()    override { m_Comp->Deserialize(m_Before); }

private:
    Component*      m_Comp;
    nlohmann::json  m_Before;
    nlohmann::json  m_After;
};