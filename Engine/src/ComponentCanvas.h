#pragma once
#include "GameObject.h"
#include "Component.h"
#include "NsCore/Ptr.h"
#include "NsGui/IView.h"
#include "NsRender/RenderDevice.h"
#include <string>

class ComponentCanvas : public Component
{
public:
    ComponentCanvas(GameObject* owner);
    ~ComponentCanvas();

    void Update() override;
    void CleanUp();

    bool IsType(ComponentType type) override { return type == ComponentType::CANVAS; }
    bool IsIncompatible(ComponentType type) override { return false; }

    void RenderToTexture();
    bool LoadXAML(const char* filename);
    void Resize(int width, int height);

    unsigned int GetTextureID() const { return textureID; }
    GameObject* GetOwner() const { return owner; }

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    void SetOpacity(float alpha);
    float GetOpacity() const;

    float opacity;
private:
    void GenerateFramebuffer(int width, int height);

private:
    std::string currentXAML;
    Noesis::Ptr<Noesis::IView> view;
    Noesis::Ptr<Noesis::RenderDevice> device;
    unsigned int fbo = 0;
    unsigned int textureID = 0;
    unsigned int rbo = 0;
    int width = 1280;
    int height = 720;
};