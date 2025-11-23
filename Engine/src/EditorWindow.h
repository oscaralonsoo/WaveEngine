#pragma once

#include <string>

// Base class for all editor windows
class EditorWindow
{
public:
    EditorWindow(const std::string& windowName)
        : name(windowName), isOpen(true) {
    }

    virtual ~EditorWindow() = default;

    // Pure virtual function - must be implemented by derived classes
    virtual void Draw() = 0;

    // Window state management
    void SetOpen(bool open) { isOpen = open; }
    bool IsOpen() const { return isOpen; }
    const std::string& GetName() const { return name; }

protected:
    std::string name;
    bool isOpen;
};