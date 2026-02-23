#pragma once

#include "EditorCommand.h"
#include <memory>
#include <stack>

class CommandHistory
{
public:
    void ExecuteCommand(std::unique_ptr<EditorCommand> command)
    {
        command->Execute();
        m_UndoStack.push(std::move(command));

        //empty stack
        while (!m_RedoStack.empty()) m_RedoStack.pop();
    }

    void PushWithoutExecute(std::unique_ptr<EditorCommand> command)
    {
        m_UndoStack.push(std::move(command));
        while (!m_RedoStack.empty())
            m_RedoStack.pop();
    }

    void Undo()
    {
        if (m_UndoStack.empty()) return;
        m_UndoStack.top()->Undo();
        m_RedoStack.push(std::move(m_UndoStack.top()));
        m_UndoStack.pop();
    }

    void Redo()
    {
        if (m_RedoStack.empty()) return;
        m_RedoStack.top()->Execute();
        m_UndoStack.push(std::move(m_RedoStack.top()));
        m_RedoStack.pop();
    }

    bool CanUndo() const { return !m_UndoStack.empty(); }
    bool CanRedo() const { return !m_RedoStack.empty(); }

    void Clear()
    {
        while (!m_UndoStack.empty()) m_UndoStack.pop();
        while (!m_RedoStack.empty()) m_RedoStack.pop();
    }

private:
    std::stack<std::unique_ptr<EditorCommand>> m_UndoStack;
    std::stack<std::unique_ptr<EditorCommand>> m_RedoStack;
};
