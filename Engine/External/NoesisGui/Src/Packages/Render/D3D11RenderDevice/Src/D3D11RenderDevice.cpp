////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


//#undef NS_LOG_TRACE
//#define NS_LOG_TRACE(...) NS_LOG_(NS_LOG_LEVEL_TRACE, __VA_ARGS__)


#include "D3D11RenderDevice.h"

#include <NsCore/Log.h>
#include <NsCore/Ptr.h>
#include <NsCore/Memory.h>
#include <NsCore/Pair.h>
#include <NsCore/HighResTimer.h>
#include <NsCore/StringUtils.h>
#include <NsApp/FastLZ.h>
#include <NsRender/Texture.h>
#include <NsRender/RenderTarget.h>

#include <stdio.h>


using namespace Noesis;
using namespace NoesisApp;


#define VS_CBUFFER0_SIZE 32 * sizeof(float)
#define VS_CBUFFER1_SIZE 4 * sizeof(float)
#define PS_CBUFFER0_SIZE 12 * sizeof(float)
#define PS_CBUFFER1_SIZE 128 * sizeof(float)

#define DX_RELEASE(o) \
    if (o != 0) \
    { \
        o->Release(); \
    }

#define DX_DESTROY(o) \
    if (o != 0) \
    { \
        ULONG refs = o->Release(); \
        NS_ASSERT(refs == 0); \
    }

#ifdef NS_PROFILE
    #define DX_BEGIN_EVENT(n) if (mGroupMarker != 0) { mGroupMarker->BeginEvent(n); }
    #define DX_END_EVENT() if (mGroupMarker != 0) { mGroupMarker->EndEvent(); }

    static const GUID WKPDID_D3DDebugObjectName_ =
    {
        0x429b8c22, 0x9188, 0x4b0c, { 0x87, 0x42, 0xac, 0xb0, 0xbf, 0x85, 0xc2, 0x00 }
    };

    static void SetDebugObjectName(ID3D11DeviceChild* resource, const char* str, ...)
    {
        char name[128];

        va_list args;
        va_start(args, str);
        vsnprintf(name, sizeof(name), str, args);
        va_end(args);

        resource->SetPrivateData(WKPDID_D3DDebugObjectName_, (UINT)strlen(name), name);
    }

    #define DX_NAME(resource, ...) SetDebugObjectName(resource, __VA_ARGS__)

#else
    #define DX_BEGIN_EVENT(n) NS_NOOP
    #define DX_END_EVENT() NS_NOOP
    #define DX_NAME(...) NS_UNUSED(__VA_ARGS__)
#endif

#define V(exp) \
    NS_MACRO_BEGIN \
        HRESULT hr_ = (exp); \
        NS_ASSERT(!FAILED(hr_)); \
    NS_MACRO_END

namespace
{

#include "Shaders.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
class D3D11Texture final: public Texture
{
public:
    D3D11Texture(ID3D11ShaderResourceView* view_, uint32_t width_, uint32_t height_,
        uint32_t levels_, bool isInverted_, bool hasAlpha_): view(view_), width(width_),
        height(height_), levels(levels_), isInverted(isInverted_), hasAlpha(hasAlpha_) {}

    ~D3D11Texture()
    {
        DX_DESTROY(view);
    }

    uint32_t GetWidth() const override { return width; }
    uint32_t GetHeight() const override { return height; }
    bool HasMipMaps() const override { return levels > 1; }
    bool IsInverted() const override { return isInverted; }
    bool HasAlpha() const override { return hasAlpha; }

    ID3D11ShaderResourceView* view;

    const uint32_t width;
    const uint32_t height;
    const uint32_t levels;
    const bool isInverted;
    const bool hasAlpha;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class D3D11RenderTarget final: public RenderTarget
{
public:
    D3D11RenderTarget(uint32_t width_, uint32_t height_, MSAA::Enum msaa_): width(width_),
        height(height_), msaa(msaa_), textureRTV(0), color(0), colorRTV(0), colorSRV(0),
        stencil(0), stencilDSV(0) {}

    ~D3D11RenderTarget()
    {
        DX_DESTROY(textureRTV);

        DX_RELEASE(color);
        DX_DESTROY(colorRTV);
        DX_DESTROY(colorSRV);

        DX_RELEASE(stencil);
        DX_DESTROY(stencilDSV);

        texture.Reset();
    }

    Texture* GetTexture() override { return texture; }

    const uint32_t width;
    const uint32_t height;
    const MSAA::Enum msaa;

    Ptr<D3D11Texture> texture;
    ID3D11RenderTargetView* textureRTV;

    ID3D11Texture2D* color;
    ID3D11RenderTargetView* colorRTV;
    ID3D11ShaderResourceView* colorSRV;

    ID3D11Texture2D* stencil;
    ID3D11DepthStencilView* stencilDSV;
};

}

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D11RenderDevice::D3D11RenderDevice(ID3D11DeviceContext* context, bool sRGB): mContext(context)
{
    mContext->GetDevice(&mDevice);
    mContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&mGroupMarker);

    FillCaps(sRGB);

    CreateBuffers();
    CreateStateObjects();
    CreateShaders();

    InvalidateStateCache();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D11RenderDevice::~D3D11RenderDevice()
{
    DestroyStateObjects();
    DestroyShaders();
    DestroyBuffers();

    DX_RELEASE(mGroupMarker);
    DX_RELEASE(mDevice);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Texture> D3D11RenderDevice::WrapTexture(ID3D11Texture2D* texture, uint32_t width,
    uint32_t height, uint32_t levels, bool isInverted, bool hasAlpha)
{
    NS_ASSERT(texture != 0);

    if (texture != 0)
    {
        ID3D11Device* device;
        texture->GetDevice(&device);

        D3D11_TEXTURE2D_DESC textureDesc;
        texture->GetDesc(&textureDesc);

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
        viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipLevels = (UINT)(-1);
        viewDesc.Texture2D.MostDetailedMip = 0;

        switch (textureDesc.Format)
        {
            case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            {
                viewDesc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
                break;
            }
            case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            {
                viewDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
                break;
            }
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            {
                viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;
            }
            case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            {
                viewDesc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
                break;
            }
            case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            {
                viewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                break;
            }
            case DXGI_FORMAT_BC1_TYPELESS:
            {
                viewDesc.Format = DXGI_FORMAT_BC1_UNORM;
                break;
            }
            case DXGI_FORMAT_BC2_TYPELESS:
            {
                viewDesc.Format = DXGI_FORMAT_BC2_UNORM;
                break;
            }
            case DXGI_FORMAT_BC3_TYPELESS:
            {
                viewDesc.Format = DXGI_FORMAT_BC3_UNORM;
                break;
            }
            case DXGI_FORMAT_BC4_TYPELESS:
            {
                viewDesc.Format = DXGI_FORMAT_BC4_UNORM;
                break;
            }
            case DXGI_FORMAT_BC5_TYPELESS:
            {
                viewDesc.Format = DXGI_FORMAT_BC5_UNORM;
                break;
            }
            case DXGI_FORMAT_BC7_TYPELESS:
            {
                viewDesc.Format = DXGI_FORMAT_BC7_UNORM;
                break;
            }
            default:
            {
                viewDesc.Format = textureDesc.Format;
                break;
            }
        }

        ID3D11ShaderResourceView* view;
        V(device->CreateShaderResourceView(texture, &viewDesc, &view));
        DX_RELEASE(device);

        return *new D3D11Texture(view, width, height, levels, isInverted, hasAlpha);
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::EnableScissorRect()
{
    mScissorStateIndex = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::DisableScissorRect()
{
    mScissorStateIndex = 2;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* D3D11RenderDevice::CreatePixelShader(const char* label, const void* hlsl, uint32_t size)
{
    // Skip signature
    hlsl = (uint8_t*)hlsl + sizeof(uint32_t);
    size -= sizeof(uint32_t);

    ID3D11PixelShader*& shader = mCustomShaders.EmplaceBack();
    V(mDevice->CreatePixelShader(hlsl, size, 0, &shader));
    DX_NAME(shader, "Noesis_%s", label);
    return (void*)(uintptr_t)mCustomShaders.Size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::ClearPixelShaders()
{
    for (ID3D11PixelShader* shader : mCustomShaders)
    {
        DX_DESTROY(shader);
    }

    mCustomShaders.Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DeviceCaps& D3D11RenderDevice::GetCaps() const
{
    return mCaps;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static MSAA::Enum ToMSAA(uint32_t sampleCount)
{
    uint32_t samples = Max(1U, Min(sampleCount, 16U));

    MSAA::Enum mssa = MSAA::x1;
    while (samples >>= 1)
    {
        mssa = (MSAA::Enum)((uint32_t)mssa + 1);
    }

    return mssa;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderTarget> D3D11RenderDevice::CreateRenderTarget(const char* label, uint32_t width,
    uint32_t height, uint32_t sampleCount, bool needsStencil)
{
    MSAA::Enum msaa = ToMSAA(mSampleDescs[(uint32_t)ToMSAA(sampleCount)].Count);

    D3D11_TEXTURE2D_DESC desc;
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc = mSampleDescs[(uint32_t)msaa];
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    ID3D11Texture2D* colorAA = 0;

    if (msaa != MSAA::x1)
    {
        if (mCaps.linearRendering)
        {
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        }
        else
        {
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        }

        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        V(mDevice->CreateTexture2D(&desc, 0, &colorAA));
        DX_NAME(colorAA, "Noesis_%s", label);
    }

    ID3D11Texture2D* stencil = nullptr;

    if (needsStencil)
    {
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        V(mDevice->CreateTexture2D(&desc, 0, &stencil));
        DX_NAME(stencil, "Noesis_%s_Stencil", label);
    }

    return CreateRenderTarget(label, width, height, msaa, colorAA, stencil);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderTarget> D3D11RenderDevice::CloneRenderTarget(const char* label, RenderTarget* surface_)
{
    D3D11RenderTarget* surface = (D3D11RenderTarget*)surface_;

    ID3D11Texture2D* colorAA = 0;
    if (surface->msaa != MSAA::x1)
    {
        colorAA = surface->color;
        colorAA->AddRef();
    }

    ID3D11Texture2D* stencil = surface->stencil;
    stencil->AddRef();

    uint32_t width = surface->width;
    uint32_t height = surface->height;
    return CreateRenderTarget(label, width, height, surface->msaa, colorAA, stencil);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static DXGI_FORMAT DXGIFormat(TextureFormat::Enum format, bool sRGB)
{
    switch (format)
    {
        case TextureFormat::RGBA8:
        case TextureFormat::RGBX8:
            return sRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::R8:
            return DXGI_FORMAT_R8_UNORM;
        default:
            NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static uint32_t TexelBytes(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM: return 4;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return 4;
        case DXGI_FORMAT_R8_UNORM: return 1;
        default: NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Texture> D3D11RenderDevice::CreateTexture(const char* label, uint32_t width, uint32_t height,
    uint32_t numLevels, TextureFormat::Enum format_, const void** data)
{
    DXGI_FORMAT format = DXGIFormat(format_, mCaps.linearRendering);

    D3D11_TEXTURE2D_DESC desc;
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = numLevels;
    desc.ArraySize = 1;
    desc.Format = format;
    desc.SampleDesc = { 1, 0 };
    desc.Usage = data ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA init[16];

    if (data)
    {
        uint32_t bpp = TexelBytes(format);

        for (uint32_t i = 0; i < numLevels; i++)
        {
            init[i].pSysMem = data[i];
            init[i].SysMemPitch = Max(width >> i, 1U) * bpp;
        }
    }

    ID3D11Texture2D* texture;
    V(mDevice->CreateTexture2D(&desc, data ? init : 0, &texture));
    DX_NAME(texture, "Noesis_%s", label);

    NS_LOG_TRACE("Texture '%s' %d x %d x %d (%s)", label, width, height, numLevels,
        [](DXGI_FORMAT format)
        {
            switch (format)
            {
                case DXGI_FORMAT_R8G8B8A8_UNORM: return "R8G8B8A8";
                case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return "R8G8B8A8_SRGB";
                case DXGI_FORMAT_R8_UNORM: return "R8";
                default: NS_ASSERT_UNREACHABLE;
            }
        }((DXGI_FORMAT)desc.Format));

    ID3D11ShaderResourceView* view;
    V(mDevice->CreateShaderResourceView(texture, 0, &view));
    DX_NAME(view, "Noesis_%s_SRV", label);
    DX_RELEASE(texture);

    bool hasAlpha = format_ == TextureFormat::RGBA8;
    return *new D3D11Texture(view, width, height, numLevels, false, hasAlpha);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::UpdateTexture(Texture* texture_, uint32_t level, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height, const void* data)
{
    NS_ASSERT(level == 0);
    D3D11Texture* texture = (D3D11Texture*)texture_;

    ID3D11Resource* resource;
    texture->view->GetResource(&resource);

    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
    texture->view->GetDesc(&desc);

    D3D11_BOX box = { x, y, 0, x + width, y + height, 1 };
    mContext->UpdateSubresource(resource, 0, &box, data, width * TexelBytes(desc.Format), 0);

    DX_RELEASE(resource);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::BeginOffscreenRender()
{
    DX_BEGIN_EVENT(L"Noesis.Offscreen");
    InvalidateStateCache();
    EnableScissorRect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::EndOffscreenRender()
{
    ClearTextures();
    DX_END_EVENT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::BeginOnscreenRender()
{
    DX_BEGIN_EVENT(L"Noesis");
    InvalidateStateCache();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::EndOnscreenRender()
{
    ClearTextures();
    DX_END_EVENT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetRenderTarget(RenderTarget* surface_)
{
    DX_BEGIN_EVENT(L"SetRenderTarget");

    ClearTextures();
    D3D11RenderTarget* surface = (D3D11RenderTarget*)surface_;
    mContext->OMSetRenderTargets(1, &surface->colorRTV, surface->stencilDSV);

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = (FLOAT)surface->width;
    viewport.Height = (FLOAT)surface->height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    mContext->RSSetViewports(1, &viewport);

#if 0
    const float color[] = { 1.0f, 0.0f, 0.0f, 0.0f };
    mContext->ClearRenderTargetView(surface->colorRTV, color);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::BeginTile(RenderTarget* surface_, const Tile& tile)
{
    D3D11RenderTarget* surface = (D3D11RenderTarget*)surface_;

    D3D11_RECT rect;
    rect.left = tile.x;
    rect.top = surface->height - (tile.y + tile.height);
    rect.right = tile.x + tile.width;
    rect.bottom = surface->height - tile.y;

    mContext->RSSetScissorRects(1, &rect);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::EndTile(RenderTarget*)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::ResolveRenderTarget(RenderTarget* surface_, const Tile* tiles, uint32_t size)
{
    D3D11RenderTarget* surface = (D3D11RenderTarget*)surface_;

    if (surface->msaa != MSAA::x1)
    {
        DX_BEGIN_EVENT(L"Resolve");

        SetInputLayout(0);
        SetVertexShader(mQuadVS);
        NS_ASSERT(surface->msaa - 1 < NS_COUNTOF(mResolvePS));
        SetPixelShader(mResolvePS[surface->msaa - 1]);

        SetRasterizerState(mRasterizerStates[0]);
        SetBlendState(mBlendStates[BlendMode::Src]);
        SetDepthStencilState(mDepthStencilStates[StencilMode::Disabled], 0);

        ClearTextures();
        mContext->OMSetRenderTargets(1, &surface->textureRTV, 0);
        SetTexture(0, surface->colorSRV);

        for (uint32_t i = 0; i < size; i++)
        {
            const Tile& tile = tiles[i];

            D3D11_RECT rect;
            rect.left = tile.x;
            rect.top = surface->height - (tile.y + tile.height);
            rect.right = tile.x + tile.width;
            rect.bottom = surface->height - tile.y;
            mContext->RSSetScissorRects(1, &rect);

            mContext->Draw(3, 0);
        }

        DX_END_EVENT();
    }

    DX_END_EVENT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* D3D11RenderDevice::MapVertices(uint32_t bytes)
{
    return MapBuffer(mVertices, bytes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::UnmapVertices()
{
    UnmapBuffer(mVertices);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* D3D11RenderDevice::MapIndices(uint32_t bytes)
{
    return MapBuffer(mIndices, bytes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::UnmapIndices()
{
    UnmapBuffer(mIndices);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::DrawBatch(const Batch& batch)
{
    NS_ASSERT(batch.shader.v < NS_COUNTOF(mPixelShaders));
    const PixelShader& psShader = mPixelShaders[batch.shader.v];

    uint32_t x = psShader.vsShader;
    NS_ASSERT(x < NS_COUNTOF(mVertexShaders));
    const VertexShader& vsShader = batch.singlePassStereo ? mVertexShadersVR[x] : mVertexShaders[x];

    SetInputLayout(vsShader.layout);
    SetVertexShader(vsShader.shader);

    if (batch.pixelShader != nullptr)
    {
        NS_ASSERT((uintptr_t)batch.pixelShader <= mCustomShaders.Size());
        SetPixelShader(mCustomShaders[(int)(uintptr_t)batch.pixelShader - 1]);
    }
    else
    {
        NS_ASSERT(psShader.shader != nullptr);
        SetPixelShader(psShader.shader);
    }

    SetBuffers(batch, vsShader.stride);
    SetRenderState(batch);
    SetTextures(batch);

    UINT startIndex = batch.startIndex + mIndices.drawPos / 2;
    UINT instanceCount = batch.singlePassStereo ? 2 : 1;
    mContext->DrawIndexedInstanced(batch.numIndices, instanceCount, startIndex, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::CreateBuffer(DynamicBuffer& buffer, uint32_t size, D3D11_BIND_FLAG flags,
    const char* label)
{
    buffer.size = size;
    buffer.pos = 0;
    buffer.drawPos = 0;
    buffer.flags = flags;

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = buffer.size;
    desc.BindFlags = buffer.flags;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    V(mDevice->CreateBuffer(&desc, 0, &buffer.obj));
    DX_NAME(buffer.obj, "Noesis_%s", label);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::CreateBuffers()
{
    memset(mVertexCBHash, 0, sizeof(mVertexCBHash));
    memset(mPixelCBHash, 0, sizeof(mPixelCBHash));

    CreateBuffer(mVertices, DYNAMIC_VB_SIZE, D3D11_BIND_VERTEX_BUFFER, "Vertices");
    CreateBuffer(mIndices, DYNAMIC_IB_SIZE, D3D11_BIND_INDEX_BUFFER, "Indices");
    CreateBuffer(mVertexCB[0], VS_CBUFFER0_SIZE, D3D11_BIND_CONSTANT_BUFFER, "VertexCB0");
    CreateBuffer(mVertexCB[1], VS_CBUFFER1_SIZE, D3D11_BIND_CONSTANT_BUFFER, "VertexCB1");
    CreateBuffer(mPixelCB[0], PS_CBUFFER0_SIZE, D3D11_BIND_CONSTANT_BUFFER, "PixelCB0");
    CreateBuffer(mPixelCB[1], PS_CBUFFER1_SIZE, D3D11_BIND_CONSTANT_BUFFER, "PixelCB1");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::DestroyBuffers()
{
    DX_DESTROY(mVertices.obj);
    DX_DESTROY(mIndices.obj);

    for (uint32_t i = 0; i < NS_COUNTOF(mVertexCB); i++)
    {
        DX_DESTROY(mVertexCB[i].obj);
    }

    for (uint32_t i = 0; i < NS_COUNTOF(mPixelCB); i++)
    {
        DX_DESTROY(mPixelCB[i].obj);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* D3D11RenderDevice::MapBuffer(DynamicBuffer& buffer, uint32_t size)
{
    NS_ASSERT(size <= buffer.size);

    // Make sure constant buffers are always discarded (no_overwrite not suported in Win7)
    if (buffer.flags == D3D11_BIND_CONSTANT_BUFFER)
    {
        buffer.pos = buffer.size;
    }

    D3D11_MAP type;

    if (buffer.pos + size > buffer.size)
    {
        type = D3D11_MAP_WRITE_DISCARD;
        buffer.pos = 0;
    }
    else
    {
        type = D3D11_MAP_WRITE_NO_OVERWRITE;
    }

    buffer.drawPos = buffer.pos;
    buffer.pos += size;

    D3D11_MAPPED_SUBRESOURCE mapped;
    V(mContext->Map(buffer.obj, 0, type, 0, &mapped));
    return (uint8_t*)mapped.pData + buffer.drawPos;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::UnmapBuffer(DynamicBuffer& buffer) const
{
    mContext->Unmap(buffer.obj, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static Pair<const char*, UINT> Semantic(uint32_t attr)
{
    switch (attr)
    {
        case Shader::Vertex::Format::Attr::Pos: return{ "POSITION", 0 };
        case Shader::Vertex::Format::Attr::Color: return { "COLOR", 0 };
        case Shader::Vertex::Format::Attr::Tex0: return { "TEXCOORD", 0 };
        case Shader::Vertex::Format::Attr::Tex1: return { "TEXCOORD", 1 };
        case Shader::Vertex::Format::Attr::Coverage: return { "COVERAGE", 0 };
        case Shader::Vertex::Format::Attr::Rect: return { "RECT", 0 };
        case Shader::Vertex::Format::Attr::Tile: return { "TILE", 0 };
        case Shader::Vertex::Format::Attr::ImagePos: return { "IMAGE_POSITION", 0 };
        default: NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static DXGI_FORMAT Format(uint32_t type)
{
    switch (type)
    {
        case Shader::Vertex::Format::Attr::Type::Float: return DXGI_FORMAT_R32_FLOAT;
        case Shader::Vertex::Format::Attr::Type::Float2: return DXGI_FORMAT_R32G32_FLOAT;
        case Shader::Vertex::Format::Attr::Type::Float4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Shader::Vertex::Format::Attr::Type::UByte4Norm: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Shader::Vertex::Format::Attr::Type::UShort4Norm: return DXGI_FORMAT_R16G16B16A16_UNORM;
        default: NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11InputLayout* D3D11RenderDevice::CreateLayout(uint32_t attributes, const void* code, uint32_t size)
{
    D3D11_INPUT_ELEMENT_DESC descs[Shader::Vertex::Format::Attr::Count];
    uint32_t n = 0;

    for (uint32_t i = 0; i < Shader::Vertex::Format::Attr::Count; i++)
    {
        if (attributes & (1 << i))
        {
            descs[n].InputSlot = 0;
            descs[n].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
            descs[n].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            descs[n].InstanceDataStepRate = 0;
            descs[n].SemanticName = Semantic(i).first;
            descs[n].SemanticIndex = Semantic(i).second;
            descs[n].Format = Format(TypeForAttr[i]);
            n++;
        }
    }

    ID3D11InputLayout* layout;
    V(mDevice->CreateInputLayout(descs, n, code, size, &layout));
    return layout;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void SetFilter(MinMagFilter::Enum minmag, MipFilter::Enum mip, D3D11_SAMPLER_DESC& desc)
{
    switch (minmag)
    {
        case MinMagFilter::Nearest:
        {
            switch (mip)
            {
                case MipFilter::Disabled:
                {
                    desc.MaxLOD = 0;
                    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
                    break;
                }
                case MipFilter::Nearest:
                {
                    desc.MaxLOD = D3D11_FLOAT32_MAX;
                    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
                    break;
                }
                case MipFilter::Linear:
                {
                    desc.MaxLOD = D3D11_FLOAT32_MAX;
                    desc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
                    break;
                }
                default:
                {
                    NS_ASSERT_UNREACHABLE;
                }
            }

            break;
        }
        case MinMagFilter::Linear:
        {
            switch (mip)
            {
                case MipFilter::Disabled:
                {
                    desc.MaxLOD = 0;
                    desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                    break;
                }
                case MipFilter::Nearest:
                {
                    desc.MaxLOD = D3D11_FLOAT32_MAX;
                    desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                    break;
                }
                case MipFilter::Linear:
                {
                    desc.MaxLOD = D3D11_FLOAT32_MAX;
                    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                    break;
                }
                default:
                {
                    NS_ASSERT_UNREACHABLE;
                }
            }

            break;
        }
        default:
        {
            NS_ASSERT_UNREACHABLE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void SetAddress(WrapMode::Enum mode, D3D_FEATURE_LEVEL level, D3D11_SAMPLER_DESC& desc)
{
    switch (mode)
    {
        case WrapMode::ClampToEdge:
        {
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            break;
        }
        case WrapMode::ClampToZero:
        {
            bool hasBorder = level >= D3D_FEATURE_LEVEL_9_3;
            desc.AddressU = hasBorder? D3D11_TEXTURE_ADDRESS_BORDER : D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = hasBorder? D3D11_TEXTURE_ADDRESS_BORDER : D3D11_TEXTURE_ADDRESS_CLAMP;
            break;
        }
        case WrapMode::Repeat:
        {
            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            break;
        }
        case WrapMode::MirrorU:
        {
            desc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            break;
        }
        case WrapMode::MirrorV:
        {
            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
            break;
        }
        case WrapMode::Mirror:
        {
            desc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
            break;
        }
        default:
        {
            NS_ASSERT_UNREACHABLE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::CreateStateObjects()
{
    // Rasterized states
    {
        D3D11_RASTERIZER_DESC desc;
        desc.CullMode = D3D11_CULL_NONE;
        desc.FrontCounterClockwise = false;
        desc.DepthBias = 0;
        desc.DepthBiasClamp = 0.0f;
        desc.SlopeScaledDepthBias = 0.0f;
        desc.DepthClipEnable = true;
        desc.MultisampleEnable = true;
        desc.AntialiasedLineEnable = false;

        desc.FillMode = D3D11_FILL_SOLID;
        desc.ScissorEnable = true;
        V(mDevice->CreateRasterizerState(&desc, &mRasterizerStates[0]));
        DX_NAME(mRasterizerStates[0], "Noesis_Solid_Scissor");

        desc.FillMode = D3D11_FILL_WIREFRAME;
        desc.ScissorEnable = true;
        V(mDevice->CreateRasterizerState(&desc, &mRasterizerStates[1]));
        DX_NAME(mRasterizerStates[1], "Noesis_Wire_Scissor");

        desc.FillMode = D3D11_FILL_SOLID;
        desc.ScissorEnable = false;
        V(mDevice->CreateRasterizerState(&desc, &mRasterizerStates[2]));
        DX_NAME(mRasterizerStates[2], "Noesis_Solid");

        desc.FillMode = D3D11_FILL_WIREFRAME;
        desc.ScissorEnable = false;
        V(mDevice->CreateRasterizerState(&desc, &mRasterizerStates[3]));
        DX_NAME(mRasterizerStates[3], "Noesis_Wire");

        DisableScissorRect();
    }

    // Blend states
    {
        D3D11_BLEND_DESC desc;
        desc.AlphaToCoverageEnable = false;
        desc.IndependentBlendEnable = false;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        // Src
        desc.RenderTarget[0].BlendEnable = false;
        V(mDevice->CreateBlendState(&desc, &mBlendStates[BlendMode::Src]));
        DX_NAME(mBlendStates[BlendMode::Src], "Noesis_Src");

        // SrcOver
        desc.RenderTarget[0].BlendEnable = true;
        V(mDevice->CreateBlendState(&desc, &mBlendStates[BlendMode::SrcOver]));
        DX_NAME(mBlendStates[BlendMode::SrcOver], "Noesis_SrcOver");

        // SrcOver_Multiply
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        V(mDevice->CreateBlendState(&desc, &mBlendStates[BlendMode::SrcOver_Multiply]));
        DX_NAME(mBlendStates[BlendMode::SrcOver_Multiply], "SrcOver_Multiply");

        // SrcOver_Screen
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
        V(mDevice->CreateBlendState(&desc, &mBlendStates[BlendMode::SrcOver_Screen]));
        DX_NAME(mBlendStates[BlendMode::SrcOver_Screen], "SrcOver_Screen");

        // SrcOver_Additive
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        V(mDevice->CreateBlendState(&desc, &mBlendStates[BlendMode::SrcOver_Additive]));
        DX_NAME(mBlendStates[BlendMode::SrcOver_Additive], "SrcOver_Additive");

        // SrcOver_Dual
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC1_COLOR;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC1_ALPHA;
        V(mDevice->CreateBlendState(&desc, &mBlendStates[BlendMode::SrcOver_Dual]));
        DX_NAME(mBlendStates[BlendMode::SrcOver_Dual], "Noesis_SrcOver_Dual");

        // Color disabled
        desc.RenderTarget[0].BlendEnable = false;
        desc.RenderTarget[0].RenderTargetWriteMask = 0;
        V(mDevice->CreateBlendState(&desc, &mBlendStateNoColor));
        DX_NAME(mBlendStateNoColor, "Noesis_NoColor");

    }

    // Depth states
    {
        D3D11_DEPTH_STENCIL_DESC desc;

        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.StencilReadMask = 0xff;
        desc.StencilWriteMask = 0xff;
        desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

        // Disabled
        desc.DepthEnable = false;
        desc.DepthFunc = D3D11_COMPARISON_NEVER;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        V(mDevice->CreateDepthStencilState(&desc, &mDepthStencilStates[StencilMode::Disabled]));
        DX_NAME(mDepthStencilStates[StencilMode::Disabled], "Noesis_Disabled");

        // Equal_Keep
        desc.DepthEnable = false;
        desc.DepthFunc = D3D11_COMPARISON_NEVER;
        desc.StencilEnable = true;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        V(mDevice->CreateDepthStencilState(&desc, &mDepthStencilStates[StencilMode::Equal_Keep]));
        DX_NAME(mDepthStencilStates[StencilMode::Equal_Keep], "Noesis_Equal_Keep");

        // Equal_Incr
        desc.DepthEnable = false;
        desc.DepthFunc = D3D11_COMPARISON_NEVER;
        desc.StencilEnable = true;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
        desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
        V(mDevice->CreateDepthStencilState(&desc, &mDepthStencilStates[StencilMode::Equal_Incr]));
        DX_NAME(mDepthStencilStates[StencilMode::Equal_Incr], "Noesis_Equal_Incr");

        // Equal_Decr
        desc.DepthEnable = false;
        desc.DepthFunc = D3D11_COMPARISON_NEVER;
        desc.StencilEnable = true;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
        desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
        V(mDevice->CreateDepthStencilState(&desc, &mDepthStencilStates[StencilMode::Equal_Decr]));
        DX_NAME(mDepthStencilStates[StencilMode::Equal_Decr], "Noesis_Equal_Decr");

        // Clear
        desc.DepthEnable = false;
        desc.DepthFunc = D3D11_COMPARISON_NEVER;
        desc.StencilEnable = true;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
        desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
        V(mDevice->CreateDepthStencilState(&desc, &mDepthStencilStates[StencilMode::Clear]));
        DX_NAME(mDepthStencilStates[StencilMode::Clear], "Noesis_Clear");

        // Disabled_ZTest
        desc.DepthEnable = true;
        desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        V(mDevice->CreateDepthStencilState(&desc, &mDepthStencilStates[StencilMode::Disabled_ZTest]));
        DX_NAME(mDepthStencilStates[StencilMode::Disabled_ZTest], "Noesis_Disabled_ZTest");

        // Equal_Keep_ZTest
        desc.DepthEnable = true;
        desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
        desc.StencilEnable = true;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        V(mDevice->CreateDepthStencilState(&desc, &mDepthStencilStates[StencilMode::Equal_Keep_ZTest]));
        DX_NAME(mDepthStencilStates[StencilMode::Equal_Keep_ZTest], "Noesis_Equal_Keep_ZTest");
    }

    // Sampler states
    {
        memset(mSamplerStates, 0, sizeof(mSamplerStates));

        D3D11_SAMPLER_DESC desc = {};
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.MaxAnisotropy = 1;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD = -D3D11_FLOAT32_MAX;
        desc.MipLODBias = -0.75f;

        const char* MinMagStr[] = { "Nearest", "Linear" };
        const char* MipStr[] = { "Disabled", "Nearest", "Linear" };
        const char* WrapStr[] = { "ClampToEdge", "ClampToZero", "Repeat", "MirrorU", "MirrorV", "Mirror" };

        D3D_FEATURE_LEVEL featureLevel = mDevice->GetFeatureLevel();

        for (uint8_t minmag = 0; minmag < MinMagFilter::Count; minmag++)
        {
            for (uint8_t mip = 0; mip < MipFilter::Count; mip++)
            {
                SetFilter(MinMagFilter::Enum(minmag), MipFilter::Enum(mip), desc);

                for (uint8_t uv = 0; uv < WrapMode::Count; uv++)
                {
                    SetAddress(WrapMode::Enum(uv), featureLevel, desc);

                    SamplerState s = {{uv, minmag, mip}};
                    NS_ASSERT(s.v < NS_COUNTOF(mSamplerStates));
                    V(mDevice->CreateSamplerState(&desc, &mSamplerStates[s.v]));
                    DX_NAME(mSamplerStates[s.v], "Noesis_%s_%s_%s", MinMagStr[minmag], MipStr[mip], WrapStr[uv]);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::DestroyStateObjects()
{
    for (uint32_t i = 0; i < NS_COUNTOF(mRasterizerStates); i++)
    {
        DX_RELEASE(mRasterizerStates[i]);
    }

    for (uint32_t i = 0; i < NS_COUNTOF(mBlendStates); i++)
    {
        DX_RELEASE(mBlendStates[i]);
    }

    DX_RELEASE(mBlendStateNoColor);

    for (uint32_t i = 0; i < NS_COUNTOF(mDepthStencilStates); i++)
    {
        DX_RELEASE(mDepthStencilStates[i]);
    }

    for (uint32_t i = 0; i < NS_COUNTOF(mSamplerStates); i++)
    {
        DX_RELEASE(mSamplerStates[i]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::CreateShaders()
{
#ifdef NS_PROFILE
    uint64_t t0 = HighResTimer::Ticks();
#endif

    uint8_t* shaders = (uint8_t*)Alloc(FastLZ::DecompressBufferSize(Shaders));
    FastLZ::Decompress(Shaders, sizeof(Shaders), shaders);

#define SHADER(n) shaders + n##_Start, n##_Size
#define VSHADER(n) case Shader::Vertex::n: \
    return stereo ? ShaderVS { #n, SHADER(n##_Stereo_VS) } : ShaderVS { #n, SHADER(n##_VS) };
#define VSHADER_SRGB(n) case Shader::Vertex::n: \
    return stereo ? \
        (sRGB ? ShaderVS{ #n, SHADER(n##_SRGB_Stereo_VS) } : ShaderVS{ #n, SHADER(n##_Stereo_VS) }) : \
        (sRGB ? ShaderVS{ #n, SHADER(n##_SRGB_VS) } : ShaderVS{ #n, SHADER(n##_VS) });

    struct ShaderVS
    {
        const char* label;
        const BYTE* code;
        uint32_t size;
    };

    auto vsShaders = [&](uint32_t shader, bool sRGB, bool stereo)
    {
        switch (shader)
        {
            VSHADER(Pos)
            VSHADER_SRGB(PosColor)
            VSHADER(PosTex0)
            VSHADER(PosTex0Rect)
            VSHADER(PosTex0RectTile)
            VSHADER_SRGB(PosColorCoverage)
            VSHADER(PosTex0Coverage)
            VSHADER(PosTex0CoverageRect)
            VSHADER(PosTex0CoverageRectTile)
            VSHADER_SRGB(PosColorTex1_SDF)
            VSHADER(PosTex0Tex1_SDF)
            VSHADER(PosTex0Tex1Rect_SDF)
            VSHADER(PosTex0Tex1RectTile_SDF)
            VSHADER_SRGB(PosColorTex1)
            VSHADER(PosTex0Tex1)
            VSHADER(PosTex0Tex1Rect)
            VSHADER(PosTex0Tex1RectTile)
            VSHADER_SRGB(PosColorTex0Tex1)
            VSHADER(PosTex0Tex1_Downsample)
            VSHADER_SRGB(PosColorTex1Rect)
            VSHADER_SRGB(PosColorTex0RectImagePos)

            default: NS_ASSERT_UNREACHABLE;
        }
    };

    memset(mLayouts, 0, sizeof(mLayouts));

    for (uint32_t i = 0; i < Shader::Vertex::Count; i++)
    {
        const ShaderVS& vsShader = vsShaders(i, mCaps.linearRendering, false);
        V(mDevice->CreateVertexShader(vsShader.code, vsShader.size, 0, &mVertexShaders[i].shader));
        DX_NAME(mVertexShaders[i].shader, "Noesis_%s", vsShader.label);

        uint32_t format = FormatForVertex[i];
        NS_ASSERT(format < NS_COUNTOF(mLayouts));

        if (mLayouts[format] == 0)
        {
            uint32_t attributes = AttributesForFormat[format];
            mLayouts[format] = CreateLayout(attributes, vsShader.code, vsShader.size);
            DX_NAME(mLayouts[format], "Noesis_%sL", vsShader.label);
        }

        mVertexShaders[i].layout = mLayouts[format];
        mVertexShaders[i].stride = SizeForFormat[format];
    }

    if (mRenderTargetArrayIndexSupported)
    {
        for (uint32_t i = 0; i < Shader::Vertex::Count; i++)
        {
            const ShaderVS& vsShader = vsShaders(i, mCaps.linearRendering, true);
            V(mDevice->CreateVertexShader(vsShader.code, vsShader.size, 0, &mVertexShadersVR[i].shader));
            DX_NAME(mVertexShadersVR[i].shader, "Noesis_%s_Stereo", vsShader.label);

            uint32_t format = FormatForVertex[i];
            NS_ASSERT(format < NS_COUNTOF(mLayouts));

            mVertexShadersVR[i].layout = mLayouts[format];
            mVertexShadersVR[i].stride = SizeForFormat[format];
        }
    }

#define PSHADER(n) case Shader::n: return ShaderPS { #n, SHADER(n##_PS) };

    struct ShaderPS
    {
        const char* label;
        const BYTE* code;
        uint32_t size;
    };

    auto psShaders = [&](uint32_t shader)
    {
        switch (shader)
        {
            PSHADER(RGBA)
            PSHADER(Mask)
            PSHADER(Clear)

            PSHADER(Path_Solid)
            PSHADER(Path_Linear)
            PSHADER(Path_Radial)
            PSHADER(Path_Pattern)
            PSHADER(Path_Pattern_Clamp)
            PSHADER(Path_Pattern_Repeat)
            PSHADER(Path_Pattern_MirrorU)
            PSHADER(Path_Pattern_MirrorV)
            PSHADER(Path_Pattern_Mirror)

            PSHADER(Path_AA_Solid)
            PSHADER(Path_AA_Linear)
            PSHADER(Path_AA_Radial)
            PSHADER(Path_AA_Pattern)
            PSHADER(Path_AA_Pattern_Clamp)
            PSHADER(Path_AA_Pattern_Repeat)
            PSHADER(Path_AA_Pattern_MirrorU)
            PSHADER(Path_AA_Pattern_MirrorV)
            PSHADER(Path_AA_Pattern_Mirror)

            PSHADER(SDF_Solid)
            PSHADER(SDF_Linear)
            PSHADER(SDF_Radial)
            PSHADER(SDF_Pattern)
            PSHADER(SDF_Pattern_Clamp)
            PSHADER(SDF_Pattern_Repeat)
            PSHADER(SDF_Pattern_MirrorU)
            PSHADER(SDF_Pattern_MirrorV)
            PSHADER(SDF_Pattern_Mirror)

            PSHADER(SDF_LCD_Solid)
            PSHADER(SDF_LCD_Linear)
            PSHADER(SDF_LCD_Radial)
            PSHADER(SDF_LCD_Pattern)
            PSHADER(SDF_LCD_Pattern_Clamp)
            PSHADER(SDF_LCD_Pattern_Repeat)
            PSHADER(SDF_LCD_Pattern_MirrorU)
            PSHADER(SDF_LCD_Pattern_MirrorV)
            PSHADER(SDF_LCD_Pattern_Mirror)

            PSHADER(Opacity_Solid)
            PSHADER(Opacity_Linear)
            PSHADER(Opacity_Radial)
            PSHADER(Opacity_Pattern)
            PSHADER(Opacity_Pattern_Clamp)
            PSHADER(Opacity_Pattern_Repeat)
            PSHADER(Opacity_Pattern_MirrorU)
            PSHADER(Opacity_Pattern_MirrorV)
            PSHADER(Opacity_Pattern_Mirror)

            PSHADER(Upsample)
            PSHADER(Downsample)

            PSHADER(Shadow)
            PSHADER(Blur)

            default: return ShaderPS{};
        }
    };

    for (uint32_t i = 0; i < Shader::Count; i++)
    {
        const ShaderPS& psShader = psShaders(i);

        mPixelShaders[i].shader = nullptr;

        if (psShader.label != nullptr)
        {
            V(mDevice->CreatePixelShader(psShader.code, psShader.size, 0, &mPixelShaders[i].shader));
            DX_NAME(mPixelShaders[i].shader, "Noesis_%s", psShader.label);
        }

        mPixelShaders[i].vsShader = VertexForShader[i];
    }

    V(mDevice->CreateVertexShader(SHADER(Quad_VS), 0, &mQuadVS));
    DX_NAME(mQuadVS, "Noesis_Quad_VS");

    V(mDevice->CreatePixelShader(SHADER(Resolve2_PS), 0, &mResolvePS[0]));
    DX_NAME(mResolvePS[0], "Noesis_Resolve2_PS");

    V(mDevice->CreatePixelShader(SHADER(Resolve4_PS), 0, &mResolvePS[1]));
    DX_NAME(mResolvePS[1], "Noesis_Resolve4_PS");

    V(mDevice->CreatePixelShader(SHADER(Resolve8_PS), 0, &mResolvePS[2]));
    DX_NAME(mResolvePS[2], "Noesis_Resolve8_PS");

    V(mDevice->CreatePixelShader(SHADER(Resolve16_PS), 0, &mResolvePS[3]));
    DX_NAME(mResolvePS[3], "Noesis_Resolve16_PS");

    Dealloc(shaders);

#ifdef NS_PROFILE
    NS_LOG_TRACE("Shaders compiled in %.0f ms", 1000.0 * HighResTimer::Seconds(
        HighResTimer::Ticks() - t0));
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::DestroyShaders()
{
    for (ID3D11InputLayout* layout : mLayouts)
    {
        DX_DESTROY(layout);
    }

    for (VertexShader& shader : mVertexShaders)
    {
        DX_DESTROY(shader.shader);
    }

    if (mRenderTargetArrayIndexSupported)
    {
        for (VertexShader& shader : mVertexShadersVR)
        {
            DX_DESTROY(shader.shader);
        }
    }

    for (PixelShader& shader : mPixelShaders)
    {
        DX_DESTROY(shader.shader);
    }

    for (ID3D11PixelShader* shader : mResolvePS)
    {
        DX_DESTROY(shader);
    }

    for (ID3D11PixelShader* shader : mCustomShaders)
    {
        DX_DESTROY(shader);
    }

    DX_DESTROY(mQuadVS);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderTarget> D3D11RenderDevice::CreateRenderTarget(const char* label, uint32_t width,
    uint32_t height, MSAA::Enum msaa, ID3D11Texture2D* colorAA, ID3D11Texture2D* stencil)
{
    Ptr<D3D11RenderTarget> surface = *new D3D11RenderTarget(width, height, msaa);

    NS_LOG_TRACE("RenderTarget '%s' %d x %d %dx", label, width, height, 1 << msaa);
    bool sRGB = mCaps.linearRendering;

    // Texture
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = sRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    ID3D11Texture2D* colorTex;
    V(mDevice->CreateTexture2D(&desc, 0, &colorTex));
    DX_NAME(colorTex, "Noesis_%s_TEX", label);

    ID3D11ShaderResourceView* viewTex;
    V(mDevice->CreateShaderResourceView(colorTex, 0, &viewTex));
    DX_NAME(viewTex, "Noesis_%s_TEX_SRV", label);
    DX_RELEASE(colorTex);

    surface->texture = *new D3D11Texture(viewTex, width, height, 1, false, true);

    V(mDevice->CreateRenderTargetView(colorTex, 0, &surface->textureRTV));
    DX_NAME(surface->textureRTV, "Noesis_%s_TEX_RTV", label);

    // Color
    if (colorAA != 0)
    {
        NS_ASSERT(msaa != MSAA::x1);
        surface->color = colorAA;
    }
    else
    {
        NS_ASSERT(msaa == MSAA::x1);
        surface->color = colorTex;
        surface->color->AddRef();
    }

    V(mDevice->CreateRenderTargetView(surface->color, 0, &surface->colorRTV));
    DX_NAME(surface->colorRTV, "Noesis_%s_RTV", label);

    V(mDevice->CreateShaderResourceView(surface->color, 0, &surface->colorSRV));
    DX_NAME(surface->colorSRV, "Noesis_%s_SRV", label);

    // Stencil
    surface->stencil = stencil;
    surface->stencilDSV = 0;

    if (stencil)
    {
        V(mDevice->CreateDepthStencilView(surface->stencil, 0, &surface->stencilDSV));
        DX_NAME(surface->stencilDSV, "Noesis_%s_DSV", label);
    }

    return surface;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::CheckMultisample()
{
    NS_ASSERT(mDevice != 0);

    unsigned int counts[MSAA::Count] = { 1, 2, 4, 8, 16 };

    for (uint32_t i = 0, last = 0; i < NS_COUNTOF(counts); i++)
    {
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
        unsigned int count = counts[i];
        unsigned int quality = 0;

        HRESULT hr = mDevice->CheckMultisampleQualityLevels(format, count, &quality);

        if (SUCCEEDED(hr) && quality > 0)
        {
            mSampleDescs[i].Count = count;
            mSampleDescs[i].Quality = 0;
            last = i;
        }
        else
        {
            mSampleDescs[i] = mSampleDescs[last];
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::FillCaps(bool sRGB)
{
    CheckMultisample();

    mCaps.centerPixelOffset = 0.0f;
    mCaps.linearRendering = sRGB;
    mCaps.subpixelRendering = true;

    mRenderTargetArrayIndexSupported = false;

    struct D3D11_FEATURE_DATA_D3D11_OPTIONS3_
    {
        BOOL VPAndRTArrayIndexFromAnyShaderFeedingRasterizer;
    };

    D3D11_FEATURE_DATA_D3D11_OPTIONS3_ options;
    if (mDevice->CheckFeatureSupport((D3D11_FEATURE)15, &options, sizeof(options)) == S_OK)
    {
        if (options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer)
        {
            mRenderTargetArrayIndexSupported = true;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::InvalidateStateCache()
{
    ID3D11Buffer* vsConstants[] = { mVertexCB[0].obj, mVertexCB[1].obj };
    mContext->VSSetConstantBuffers(0, 2, vsConstants);

    ID3D11Buffer* psConstants[] = { mPixelCB[0].obj, mPixelCB[1].obj };
    mContext->PSSetConstantBuffers(0, 2, psConstants);

    mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mContext->HSSetShader(nullptr, nullptr, 0);
    mContext->DSSetShader(nullptr, nullptr, 0);
    mContext->GSSetShader(nullptr, nullptr, 0);
    mContext->CSSetShader(nullptr, nullptr, 0);
    mContext->SetPredication(nullptr, 0);

    mIndexBuffer = nullptr;
    mLayout = nullptr;
    mVertexShader = nullptr;
    mPixelShader = nullptr;
    mRasterizerState = nullptr;
    mBlendState = nullptr;
    mDepthStencilState = nullptr;
    mStencilRef = (unsigned int)-1;
    memset(mTextures, 0, sizeof(mTextures));
    memset(mSamplers, 0, sizeof(mSamplers));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetIndexBuffer(ID3D11Buffer* buffer)
{
    if (mIndexBuffer != buffer)
    {
        mContext->IASetIndexBuffer(buffer, DXGI_FORMAT_R16_UINT, 0);
        mIndexBuffer = buffer;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetVertexBuffer(ID3D11Buffer* buffer, uint32_t stride, uint32_t offset)
{
    mContext->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetInputLayout(ID3D11InputLayout* layout)
{
    if (layout != mLayout)
    {
        mContext->IASetInputLayout(layout);
        mLayout = layout;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetVertexShader(ID3D11VertexShader* shader)
{
    if (shader != mVertexShader)
    {
        mContext->VSSetShader(shader, 0, 0);
        mVertexShader = shader;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetPixelShader(ID3D11PixelShader* shader)
{
    if (shader != mPixelShader)
    {
        mContext->PSSetShader(shader, 0, 0);
        mPixelShader = shader;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetRasterizerState(ID3D11RasterizerState* state)
{
    if (state != mRasterizerState)
    {
        mContext->RSSetState(state);
        mRasterizerState = state;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetBlendState(ID3D11BlendState* state)
{
    if (state != mBlendState)
    {
        mContext->OMSetBlendState(state, 0, 0xffffffff);
        mBlendState = state;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetDepthStencilState(ID3D11DepthStencilState* state, unsigned int stencilRef)
{
    if (state != mDepthStencilState || stencilRef != mStencilRef)
    {
        mContext->OMSetDepthStencilState(state, stencilRef);
        mDepthStencilState = state;
        mStencilRef = stencilRef;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetSampler(unsigned int slot, ID3D11SamplerState* sampler)
{
    NS_ASSERT(slot < NS_COUNTOF(mSamplers));
    if (sampler != mSamplers[slot])
    {
        mContext->PSSetSamplers(slot, 1, &sampler);
        mSamplers[slot] = sampler;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetTexture(unsigned int slot, ID3D11ShaderResourceView* texture)
{
    NS_ASSERT(slot < NS_COUNTOF(mTextures));
    if (texture != mTextures[slot])
    {
        mContext->PSSetShaderResources(slot, 1, &texture);
        mTextures[slot] = texture;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::ClearTextures()
{
    memset(mTextures, 0, sizeof(mTextures));
    mContext->PSSetShaderResources(0, NS_COUNTOF(mTextures), mTextures);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetBuffers(const Batch& batch, uint32_t stride)
{
    // Indices
    SetIndexBuffer(mIndices.obj);

    // Vertices
    uint32_t offset = mVertices.drawPos + batch.vertexOffset;
    SetVertexBuffer(mVertices.obj, stride, offset);

    // Vertex Shader Constant Buffers
    static_assert(NS_COUNTOF(mVertexCB) == NS_COUNTOF(Batch::vertexUniforms), "");
    static_assert(NS_COUNTOF(mVertexCBHash) == NS_COUNTOF(Batch::vertexUniforms), "");

    for (uint32_t i = 0; i < NS_COUNTOF(mVertexCB); i++)
    {
        if (batch.vertexUniforms[i].numDwords > 0)
        {
            if (mVertexCBHash[i] != batch.vertexUniforms[i].hash)
            {
                uint32_t size = batch.vertexUniforms[i].numDwords * sizeof(uint32_t);
                void* ptr = MapBuffer(mVertexCB[i], size);
                memcpy(ptr, batch.vertexUniforms[i].values, size);
                UnmapBuffer(mVertexCB[i]);

                mVertexCBHash[i] = batch.vertexUniforms[i].hash;
            }
        }
    }

    // Pixel Shader Constant Buffers
    static_assert(NS_COUNTOF(mPixelCB) == NS_COUNTOF(Batch::pixelUniforms), "");
    static_assert(NS_COUNTOF(mPixelCBHash) == NS_COUNTOF(Batch::vertexUniforms), "");

    for (uint32_t i = 0; i < NS_COUNTOF(mPixelCB); i++)
    {
        if (batch.pixelUniforms[i].numDwords > 0)
        {
            if (mPixelCBHash[i] != batch.pixelUniforms[i].hash)
            {
                uint32_t size = batch.pixelUniforms[i].numDwords * sizeof(uint32_t);
                void* ptr = MapBuffer(mPixelCB[i], size);
                memcpy(ptr, batch.pixelUniforms[i].values, size);
                UnmapBuffer(mPixelCB[i]);

                mPixelCBHash[i] = batch.pixelUniforms[i].hash;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetRenderState(const Batch& batch)
{
    auto f = batch.renderState.f;

    NS_ASSERT(f.wireframe < NS_COUNTOF(mRasterizerStates));
    ID3D11RasterizerState* rasterizer = mRasterizerStates[mScissorStateIndex + f.wireframe];
    SetRasterizerState(rasterizer);

    NS_ASSERT(f.blendMode < NS_COUNTOF(mBlendStates));
    ID3D11BlendState* blend = f.colorEnable ? mBlendStates[f.blendMode] : mBlendStateNoColor;
    SetBlendState(blend);

    NS_ASSERT(f.stencilMode < NS_COUNTOF(mDepthStencilStates));
    ID3D11DepthStencilState* stencil = mDepthStencilStates[f.stencilMode];
    SetDepthStencilState(stencil, batch.stencilRef);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11RenderDevice::SetTextures(const Batch& batch)
{
    if (batch.pattern != 0)
    {
        D3D11Texture* t = (D3D11Texture*)batch.pattern;
        SetTexture(TextureSlot::Pattern, t->view);
        SetSampler(TextureSlot::Pattern, mSamplerStates[batch.patternSampler.v]);
    }

    if (batch.ramps != 0)
    {
        D3D11Texture* t = (D3D11Texture*)batch.ramps;
        SetTexture(TextureSlot::Ramps, t->view);
        SetSampler(TextureSlot::Ramps, mSamplerStates[batch.rampsSampler.v]);
    }

    if (batch.image != 0)
    {
        D3D11Texture* t = (D3D11Texture*)batch.image;
        SetTexture(TextureSlot::Image, t->view);
        SetSampler(TextureSlot::Image, mSamplerStates[batch.imageSampler.v]);
    }

    if (batch.glyphs != 0)
    {
        D3D11Texture* t = (D3D11Texture*)batch.glyphs;
        SetTexture(TextureSlot::Glyphs, t->view);
        SetSampler(TextureSlot::Glyphs, mSamplerStates[batch.glyphsSampler.v]);
    }

    if (batch.shadow != 0)
    {
        D3D11Texture* t = (D3D11Texture*)batch.shadow;
        SetTexture(TextureSlot::Shadow, t->view);
        SetSampler(TextureSlot::Shadow, mSamplerStates[batch.shadowSampler.v]);
    }
}
