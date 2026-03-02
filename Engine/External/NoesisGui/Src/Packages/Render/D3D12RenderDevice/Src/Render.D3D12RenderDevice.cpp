////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/Package.h>
#include <NsRender/D3D12Factory.h>
#include <NsRender/Texture.h>

#include "D3D12RenderDevice.h"


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderDevice> D3D12Factory::CreateDevice(ID3D12Device* device, ID3D12Fence* frameFence,
    DXGI_FORMAT colorFormat, DXGI_FORMAT stencilFormat, DXGI_SAMPLE_DESC sampleDesc, bool sRGB)
{
    return *new D3D12RenderDevice(device, frameFence, colorFormat, stencilFormat, sampleDesc, sRGB);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Texture> D3D12Factory::WrapTexture(ID3D12Resource* texture, uint32_t width, uint32_t height,
    uint32_t levels, bool isInverted, bool hasAlpha, uint32_t state)
{
    return D3D12RenderDevice::WrapTexture(texture, width, height, levels, isInverted, hasAlpha,
        (D3D12_RESOURCE_STATES)state);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12Factory::SetCommandList(RenderDevice* device_, ID3D12GraphicsCommandList* commands,
    uint64_t safeFenceValue)
{
    D3D12RenderDevice* device = (D3D12RenderDevice*)device_;
    device->SetCommandList(commands, safeFenceValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* D3D12Factory::CreatePixelShader(RenderDevice* device_, const char* label, uint8_t shader,
    const void* hlsl, uint32_t size)
{
    D3D12RenderDevice* device = (D3D12RenderDevice*)device_;
    return device->CreatePixelShader(label, shader, hlsl, size);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12Factory::ClearPixelShaders(RenderDevice* device_)
{
    D3D12RenderDevice* device = (D3D12RenderDevice*)device_;
    device->ClearPixelShaders();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D12Resource* D3D12Factory::GetNativePtr(Texture* texture)
{
    return D3D12RenderDevice::GetNativePtr(texture);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D12_RESOURCE_STATES D3D12Factory::GetTextureState(Texture* texture)
{
    return D3D12RenderDevice::GetTextureState(texture);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12Factory::SetTextureState(Texture* texture, D3D12_RESOURCE_STATES state)
{
    D3D12RenderDevice::SetTextureState(texture, state);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12Factory::EndPendingSplitBarriers(RenderDevice* device_)
{
    D3D12RenderDevice* device = (D3D12RenderDevice*)device_;
    device->EndPendingSplitBarriers();
}

NS_BEGIN_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_REGISTER_REFLECTION(Render, D3D12RenderDevice)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_INIT_PACKAGE(Render, D3D12RenderDevice)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_SHUTDOWN_PACKAGE(Render, D3D12RenderDevice)
{
}

NS_END_COLD_REGION
