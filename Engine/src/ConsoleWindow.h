#pragma once

#include "EditorWindow.h"

class ConsoleWindow : public EditorWindow
{
public:
    ConsoleWindow();
    ~ConsoleWindow() override = default;

    void Draw() override;

private:
    // Filter flags
    bool showErrors = true;
    bool showWarnings = true;
    bool showInfo = true;
    bool showSuccess = true;
    bool showLoading = true;
    bool showLibraryInfo = true;

    // Scroll control
    bool autoScroll = true;
    bool scrollToBottom = false;
};