#pragma once

class EditorCommand
{
public:
    virtual ~EditorCommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};
