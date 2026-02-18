#pragma once
#include "Module.h"
#include <cstdint>
#include "NsCore/Ptr.h"
#include "NsGui/IView.h"

class UI : public Module
{
public:
    UI();
    ~UI();

    bool Start() override;
    bool PreUpdate() override;
    bool Update() override;
    bool CleanUp() override;

    void OnResize(uint32_t width, uint32_t height);

private:
    Noesis::Ptr<Noesis::IView> m_view;

};