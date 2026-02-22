#pragma once
#include "Module.h"
#include <cstdint>
#include <vector>
#include "NsCore/Ptr.h"
#include "NsRender/RenderDevice.h"

class ComponentCanvas;

class UI : public Module
{
public:
    UI();
    ~UI();
    bool Start() override;
    bool CleanUp() override;
    void OnResize(uint32_t width, uint32_t height);

    void SetMousePosition(int x, int y);
    void OnMouseButtonDown(int x, int y, int sdlButton);
    void OnMouseButtonUp(int x, int y, int sdlButton);
    void OnMouseWheel(int x, int y, int delta);

    std::vector<ComponentCanvas*> GetCanvas() { return canvas; }
    Noesis::Ptr<Noesis::RenderDevice> GetRenderDevice() { return renderDevice; }
    void RegisterCanvas(ComponentCanvas* c) { canvas.push_back(c); }
    void UnregisterCanvas(ComponentCanvas* c) {
        canvas.erase(std::remove(canvas.begin(), canvas.end(), c), canvas.end());
    }

private:
    Noesis::Ptr<Noesis::RenderDevice> renderDevice;
    std::vector<ComponentCanvas*> canvas;
    int mouseX = 0, mouseY = 0;
};