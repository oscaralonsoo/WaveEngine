#pragma once

#include "EditorWindow.h"

class ConsoleWindow : public EditorWindow
{
public:
    ConsoleWindow();
    ~ConsoleWindow() override = default;

    void Draw() override;

    void FlashError();

private:
    // Filter flags
    bool showErrors = true;
    bool showWarnings = true;
    bool showInfo = true;
    bool showSuccess = true;
    bool showLoading = true;
    bool showLibraryInfo = true;
    bool showShaders = true;

    // Scroll control
    bool autoScroll = true;
    bool scrollToBottom = false;

    // Error indicator
    float errorFlashTimer = 0.0f;
    static constexpr float FLASH_DURATION = 2.0f;
};