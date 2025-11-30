#pragma once

#include "EditorWindow.h"
#include <vector>

class ConfigurationWindow : public EditorWindow
{
public:
    ConfigurationWindow();
    ~ConfigurationWindow() override = default;

    void Draw() override;

    // Getters for debug visualization flags
    bool ShouldShowAABB() const { return showAABB; }
    bool ShouldShowOctree() const { return showOctree; }
    bool ShouldShowRaycast() const { return showRaycast; }
    bool ShouldShowZBuffer() const { return showZBuffer; }

private:
    void DrawFPSGraph();
    void DrawHardwareInfo();
    void DrawWindowSettings();
    void DrawCameraSettings();
    void DrawRendererSettings();

    // FPS tracking
    std::vector<float> fpsHistory;
    const int maxFPSHistory = 100;
    float fpsTimer = 0.0f;

    // Configuration state
    bool fullscreen = false;
    float brightness = 1.0f;

    // Camera settings cache
    float cameraMovementSpeed = 2.5f;
    float cameraMouseSensitivity = 0.2f;
    float cameraScrollSpeed = 0.5f;
    float cameraFOV = 45.0f;
    float cameraPanSensitivity = 0.003f;

    // Debug visualization
    bool showAABB = false;
    bool showOctree = false;
    bool showRaycast = false;
    bool showZBuffer = false;
};