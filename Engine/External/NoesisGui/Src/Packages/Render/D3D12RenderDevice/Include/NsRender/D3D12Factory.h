////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __RENDER_D3D12FACTORY_H__
#define __RENDER_D3D12FACTORY_H__


#include <NsCore/Noesis.h>
#include <NsRender/D3D12RenderDeviceApi.h>

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4471)

struct ID3D12Device;
struct ID3D12Fence;
struct ID3D12Resource;
struct ID3D12GraphicsCommandList;
struct DXGI_SAMPLE_DESC;
enum DXGI_FORMAT;
enum D3D12_RESOURCE_STATES;

NS_WARNING_POP

namespace Noesis
{
class RenderDevice;
class Texture;
template<class T> class Ptr;
}

namespace NoesisApp
{

struct NS_RENDER_D3D12RENDERDEVICE_API D3D12Factory
{
    static Noesis::Ptr<Noesis::RenderDevice> CreateDevice(ID3D12Device* device,
        ID3D12Fence* frameFence, DXGI_FORMAT colorFormat, DXGI_FORMAT stencilFormat,
        DXGI_SAMPLE_DESC sampleDesc, bool sRGB);
    static Noesis::Ptr<Noesis::Texture> WrapTexture(ID3D12Resource* texture, uint32_t width,
        uint32_t height, uint32_t levels, bool isInverted, bool hasAlpha, uint32_t state);
    static void SetCommandList(Noesis::RenderDevice* device, ID3D12GraphicsCommandList* commands,
        uint64_t safeFenceValue);
    static void* CreatePixelShader(Noesis::RenderDevice* device, const char* label, uint8_t shader,
        const void* hlsl, uint32_t size);
    static void ClearPixelShaders(Noesis::RenderDevice* device);

    static ID3D12Resource* GetNativePtr(Noesis::Texture* texture);
    static D3D12_RESOURCE_STATES GetTextureState(Noesis::Texture* texture);
    static void SetTextureState(Noesis::Texture* texture, D3D12_RESOURCE_STATES state);

    static void EndPendingSplitBarriers(Noesis::RenderDevice* device);
};

}

#endif
