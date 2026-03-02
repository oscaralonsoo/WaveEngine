#pragma once

#include "EditorCommand.h"
#include <vector>
#include <memory>

class CompositeCommand : public EditorCommand
{
public:
    void AddCommand(std::unique_ptr<EditorCommand> command)
    {
        m_Commands.push_back(std::move(command));
    }

    bool IsEmpty() const { return m_Commands.empty(); }

    void Execute() override
    {
        for (auto& cmd : m_Commands)
            cmd->Execute();
    }

    void Undo() override
    {
        for (int i = static_cast<int>(m_Commands.size()) - 1; i >= 0; --i)
            m_Commands[i]->Undo();
    }

private:
    std::vector<std::unique_ptr<EditorCommand>> m_Commands;
};
