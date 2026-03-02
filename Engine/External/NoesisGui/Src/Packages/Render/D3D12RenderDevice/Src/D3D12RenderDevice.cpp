////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


//#undef NS_LOG_TRACE
//#define NS_LOG_TRACE(...) NS_LOG_(NS_LOG_LEVEL_TRACE, __VA_ARGS__)


#include "D3D12RenderDevice.h"

#include <NsRender/Texture.h>
#include <NsRender/RenderTarget.h>
#include <NsCore/HighResTimer.h>
#include <NsCore/String.h>
#include <NsCore/Log.h>
#include <NsCore/Pair.h>
#include <NsApp/FastLZ.h>

#include <stdio.h>
#include <shlobj.h>

#ifdef NS_PLATFORM_GAME_CORE
  #ifndef D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
    #define D3D12_TEXTURE_DATA_PITCH_ALIGNMENT D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT
  #endif
  #include <pix3.h>

  // For Xbox using DXC is recommended as shaders are precompiled (IL stripped)
  #define USE_DXC_SHADERS
#else
    #include "directx/pix.h"
#endif


using namespace Noesis;
using namespace NoesisApp;


#define PREALLOCATED_DYNAMIC_PAGES 1
#define CBUFFER_SIZE 16 * 1024
#define MAX_RTV_DESCRIPTORS 128
#define MAX_DSV_DESCRIPTORS 128
#define MAX_SRV_DESCRIPTORS 1536


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

#ifdef NS_PLATFORM_GAME_CORE
    #define DX_ARGS(_x) IID_GRAPHICS_PPV_ARGS(_x)
#else
    #define DX_ARGS(_x) IID_PPV_ARGS(_x)
#endif

#ifdef NS_PROFILE
    static uint32_t gFlipCount;

    static void SetDebugObjectName(ID3D12Object* obj, const char* str, ...)
    {
        char name[128];

        va_list args;
        va_start(args, str);
        vsnprintf(name, sizeof(name), str, args);
        va_end(args);

        WCHAR name_[128];
        MultiByteToWideChar(CP_UTF8, 0, name, -1, name_, 128);

        obj->SetName(name_);
    }

    #define DX_BEGIN_EVENT(...) PIXBeginEvent(mCommands, 0, __VA_ARGS__)
    #define DX_END_EVENT() PIXEndEvent(mCommands)
    #define DX_NAME(resource, ...) SetDebugObjectName(resource, __VA_ARGS__)

#else
    #define DX_BEGIN_EVENT(...) NS_NOOP
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

#ifdef USE_DXC_SHADERS
  #define STEREO_RENDERING 0
  #if defined(NS_PLATFORM_XBOX_ONE_CORE)
    #include "Shaders_x.h"
  #elif defined(NS_PLATFORM_SCARLETT)
    #include "Shaders_xs.h"
  #endif
#else
  #include "Shaders.h"
  #define STEREO_RENDERING 1
#endif

#define SHADER(n) ShaderVS { n##_VS_Start, n##_VS_Size }
#define SHADER_SRGB(n) ShaderVS { n##_SRGB_VS_Start, n##_SRGB_VS_Size }
#define SHADER_STEREO(n) ShaderVS { n##_Stereo_VS_Start, n##_Stereo_VS_Size }
#define SHADER_SRGB_STEREO(n) ShaderVS { n##_SRGB_Stereo_VS_Start, n##_SRGB_Stereo_VS_Size }

#if STEREO_RENDERING
    #define VSHADER(n) case Shader::Vertex::n: return stereo ? SHADER_STEREO(n) : SHADER(n);
    #define VSHADER_SRGB(n) case Shader::Vertex::n: \
        return stereo ? \
            (sRGB ? SHADER_SRGB_STEREO(n) : SHADER_STEREO(n)) : \
            (sRGB ? SHADER_SRGB(n) : SHADER(n));
#else
    #define VSHADER(n) case Shader::Vertex::n: return SHADER(n);
    #define VSHADER_SRGB(n) case Shader::Vertex::n: return sRGB ? SHADER_SRGB(n) : SHADER(n);
#endif

struct ShaderVS
{
    uint32_t start;
    uint32_t size;
};

#ifdef USE_DXC_SHADERS
struct Vertex_x
{
    enum Enum
    {
        Pos_x = Shader::Vertex::Count,
        PosTex0_x,
        PosTex0Coverage_x,
        PosTex0Tex1_SDF_x,
        PosTex0Tex1_x,
        PosColorTex1_x
    };
};

static constexpr const uint8_t VertexForShader_x[Noesis::Shader::Count] =
{
    21, /*RGBA*/
    0,  /*Mask*/
    0,  /*Clear*/

    1,  /*Path_Solid*/
    2,  /*Path_Linear*/
    2,  /*Path_Radial*/
    22, /*Path_Pattern*/
    3,  /*Path_Pattern_Clamp*/
    4,  /*Path_Pattern_Repeat*/
    4,  /*Path_Pattern_MirrorU*/
    4,  /*Path_Pattern_MirrorV*/
    4,  /*Path_Pattern_Mirror*/

    5,  /*Path_AA_Solid*/
    6,  /*Path_AA_Linear*/
    6,  /*Path_AA_Radial*/
    23, /*Path_AA_Pattern*/
    7,  /*Path_AA_Pattern_Clamp*/
    8,  /*Path_AA_Pattern_Repeat*/
    8,  /*Path_AA_Pattern_MirrorU*/
    8,  /*Path_AA_Pattern_MirrorV*/
    8,  /*Path_AA_Pattern_Mirror*/

    9,  /*SDF_Solid*/
    10, /*SDF_Linear*/
    10, /*SDF_Radial*/
    24, /*SDF_Pattern*/
    11, /*SDF_Pattern_Clamp*/
    12, /*SDF_Pattern_Repeat*/
    12, /*SDF_Pattern_MirrorU*/
    12, /*SDF_Pattern_MirrorV*/
    12, /*SDF_Pattern_Mirror*/

    9,  /*SDF_LCD_Solid*/
    10, /*SDF_LCD_Linear*/
    10, /*SDF_LCD_Radial*/
    10, /*SDF_LCD_Pattern*/
    11, /*SDF_LCD_Pattern_Clamp*/
    12, /*SDF_LCD_Pattern_Repeat*/
    12, /*SDF_LCD_Pattern_MirrorU*/
    12, /*SDF_LCD_Pattern_MirrorV*/
    12, /*SDF_LCD_Pattern_Mirror*/

    13, /*Opacity_Solid*/
    14, /*Opacity_Linear*/
    14, /*Opacity_Radial*/
    25, /*Opacity_Pattern*/
    15, /*Opacity_Pattern_Clamp*/
    16, /*Opacity_Pattern_Repeat*/
    16, /*Opacity_Pattern_MirrorU*/
    16, /*Opacity_Pattern_MirrorV*/
    16, /*Opacity_Pattern_Mirror*/

    17, /*Upsample*/
    18, /*Downsample*/

    19, /*Shadow*/
    26, /*Blur*/

    20, /*Custom_Effect*/
};

#define VSHADER_X(n) case Vertex_x::n: return ShaderVS { n##_VS_Start, n##_VS_Size };
#endif

static auto ShadersVS = [](uint32_t shader, bool sRGB, bool stereo)
{
  #if !STEREO_RENDERING
    NS_ASSERT(!stereo);
  #endif

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

      #ifdef USE_DXC_SHADERS
        VSHADER_X(Pos_x)
        VSHADER_X(PosTex0_x)
        VSHADER_X(PosTex0Coverage_x)
        VSHADER_X(PosTex0Tex1_SDF_x)
        VSHADER_X(PosTex0Tex1_x)
        VSHADER_X(PosColorTex1_x)
      #endif

        default: NS_ASSERT_UNREACHABLE;
    }
};

// Root signature flags
static const uint32_t VS_CB0 = 1;
static const uint32_t VS_CB1 = 2;
static const uint32_t PS_CB0 = 4;
static const uint32_t PS_CB1 = 8;
static const uint32_t PS_T0 = 16;
static const uint32_t PS_T1 = 32;
static const uint32_t PS_T2 = 64;
static const uint32_t PS_T3 = 128;
static const uint32_t PS_T4 = 256;

////////////////////////////////////////////////////////////////////////////////////////////////////
class D3D12Texture final: public Texture
{
public:
    ~D3D12Texture()
    {
        if (device)
        {
            device->DeviceDestroyed() -= MakeDelegate(this, &D3D12Texture::OnDestroyDevice);
            device->SafeReleaseTexture(this);
        }
        else
        {
            obj->Release();
        }
    }

    void LinkDevice(D3D12RenderDevice* device_, uint32_t srv_)
    {
        NS_ASSERT(device == 0);
        NS_ASSERT(device_ != 0);
        device = device_;
        srv = srv_;

        device->DeviceDestroyed() += MakeDelegate(this, &D3D12Texture::OnDestroyDevice);
    }

    void OnDestroyDevice(RenderDevice* device_)
    {
        NS_ASSERT(device == device_);
        device = nullptr;
    }

    uint32_t GetWidth() const override { return width; }
    uint32_t GetHeight() const override { return height; }
    bool HasMipMaps() const override { return levels > 1; }
    bool IsInverted() const override { return isInverted; }
    bool HasAlpha() const override { return hasAlpha; }

    ID3D12Resource* obj = nullptr;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    bool pendingBarrierToSRV = false;

    D3D12RenderDevice* device = nullptr;
    uint32_t srv = 0;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t levels = 0;
    bool isInverted = false;
    bool hasAlpha = false;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class D3D12RenderTarget final: public RenderTarget
{
public:
    ~D3D12RenderTarget()
    {
        texture->device->SafeReleaseRenderTarget(this);
    }

    Texture* GetTexture() override { return texture; }

    uint32_t width = 0;
    uint32_t height = 0;

    uint32_t rtv = 0;
    uint32_t dsv = 0;

    Ptr<D3D12Texture> texture;
    ID3D12Resource* colorAA = nullptr;
    ID3D12Resource* stencil = nullptr;
};

}

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D12RenderDevice::D3D12RenderDevice(ID3D12Device* device, ID3D12Fence* frameFence,
    DXGI_FORMAT colorFormat, DXGI_FORMAT stencilFormat, DXGI_SAMPLE_DESC sampleDesc, bool sRGB):
    mDevice(device), mFrameFence(frameFence), mCommands(nullptr), mSafeFenceValue(0),
    mColorFormat(colorFormat), mStencilFormat(stencilFormat), mSampleDesc(sampleDesc)
{
  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    HMODULE d3d12 = LoadLibraryA("d3d12.dll");
    NS_ASSERT(d3d12 != 0);
    D3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)
        GetProcAddress(d3d12, "D3D12SerializeRootSignature");
    NS_ASSERT(D3D12SerializeRootSignature != 0);
    D3D12SerializeVersionedRootSignature = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)
        GetProcAddress(d3d12, "D3D12SerializeVersionedRootSignature");

    mLibrary = nullptr;
    mLibraryUpdated = false;
    mMappedLibrary = nullptr;
    ID3D12Device1* device1 = nullptr;

    if (SUCCEEDED(mDevice->QueryInterface(DX_ARGS(&device1))))
    {
        ID3D12InfoQueue* infoQueue = nullptr;
        if (SUCCEEDED(mDevice->QueryInterface(IID_PPV_ARGS(&infoQueue))))
        {
            // These errors are properly handled below
            D3D12_MESSAGE_ID messages[] =
            {
                D3D12_MESSAGE_ID_CREATEPIPELINELIBRARY_INVALIDLIBRARYBLOB,
                D3D12_MESSAGE_ID_CREATEPIPELINELIBRARY_DRIVERVERSIONMISMATCH,
                D3D12_MESSAGE_ID_CREATEPIPELINELIBRARY_ADAPTERVERSIONMISMATCH,
                D3D12_MESSAGE_ID_CREATEPIPELINELIBRARY_UNSUPPORTED,
            };

            D3D12_INFO_QUEUE_FILTER filter{};
            filter.DenyList.NumIDs = NS_COUNTOF(messages);
            filter.DenyList.pIDList = messages;
            infoQueue->PushCopyOfStorageFilter();
            infoQueue->AddStorageFilterEntries(&filter);
        }

        // Pipeline cache filename
        HRESULT hr = SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, mPipelineCachePath);
        NS_ASSERT(!FAILED(hr));

        wcscat_s(mPipelineCachePath, L"\\Noesis Technologies");
        CreateDirectory(mPipelineCachePath, nullptr);

        wcscat_s(mPipelineCachePath, L"\\D3D12");
        CreateDirectory(mPipelineCachePath, nullptr);

        wcscat_s(mPipelineCachePath, L"\\PipelineLibrary.bin");

        LARGE_INTEGER size{};
        HANDLE file = CreateFile(mPipelineCachePath, GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (file != INVALID_HANDLE_VALUE)
        {
            HANDLE mapping = CreateFileMapping(file, nullptr, PAGE_READONLY, 0, 0, nullptr);

            if (mapping != nullptr)
            {
                mMappedLibrary = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
                V(CloseHandle(mapping));

                if (mMappedLibrary != 0)
                {
                    V(GetFileSizeEx(file, &size));
                }
            }

            V(CloseHandle(file));
        }
        else
        {
            NS_LOG_WARNING("Pipeline library cache not found");
        }

        hr = device1->CreatePipelineLibrary(mMappedLibrary, size.LowPart, DX_ARGS(&mLibrary));

        if (FAILED(hr))
        {
            if (mMappedLibrary)
            {
                V(UnmapViewOfFile(mMappedLibrary));
                mMappedLibrary = nullptr;
            }

            if (hr == DXGI_ERROR_UNSUPPORTED)
            {
                NS_LOG_WARNING("Pipeline library not supported");
            }
            else
            {
                // Create a fresh library if
                //  - D3D12_ERROR_DRIVER_VERSION_MISMATCH (data came from different hardware)
                //  - D3D12_ERROR_ADAPTER_NOT_FOUND (provided data came from an old driver or runtime)
                //  - E_INVALIDARG (blob is corrupted or unrecognized)
                NS_LOG_WARNING("Regenerating Pipeline library cache");
                device1->CreatePipelineLibrary(nullptr, 0, DX_ARGS(&mLibrary));
            }
        }

        if (mLibrary)
        {
            DX_NAME(mLibrary, "Noesis_Library");
        }

        if (infoQueue)
        {
            infoQueue->PopStorageFilter();
            DX_RELEASE(infoQueue);
        }

        DX_RELEASE(device1);
    }
  #endif

    FillCaps(sRGB);

    CreateBuffers();
    CreateHeaps();
    CreateShaders();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D12RenderDevice::~D3D12RenderDevice()
{
    ProcessPendingReleases(true);
    NS_ASSERT(mPendingReleases.Empty());

    DestroyShaders();
    DestroyBuffers();

    DX_DESTROY(mRTVHeap);
    DX_DESTROY(mDSVHeap);
    DX_DESTROY(mSRVHeap);
    DX_DESTROY(mSamplerHeap);

  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    if (mLibrary)
    {
        if (mLibraryUpdated)
        {
            SIZE_T size = mLibrary->GetSerializedSize();
            void* memory = Alloc(size);
            mLibrary->Serialize(memory, size);
            DX_DESTROY(mLibrary);

            if (mMappedLibrary)
            {
                V(UnmapViewOfFile(mMappedLibrary));
            }

            FILE* fp = nullptr;
            _wfopen_s(&fp, mPipelineCachePath, L"wb");

            if (fp)
            {
                fwrite(memory, size, 1, fp);
                fclose(fp);

                NS_LOG_INFO("Saved pipeline library cache (%zu bytes)", size);
            }
            else
            {
                NS_LOG_WARNING("Failed to save pipeline library cache (%zu bytes)", size);
            }

            Dealloc(memory);
        }
        else
        {
            DX_DESTROY(mLibrary);

            if (mMappedLibrary)
            {
                V(UnmapViewOfFile(mMappedLibrary));
            }
        }
    }
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void RasterizerDesc(D3D12_RASTERIZER_DESC& desc, RenderState state, BaseString& label)
{
    memset(&desc, 0, sizeof(desc));
    desc.CullMode = D3D12_CULL_MODE_NONE;

    if (state.f.wireframe)
    {
        label += "_Wire";
        desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
    }
    else
    {
        desc.FillMode = D3D12_FILL_MODE_SOLID;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void BlendDesc(D3D12_BLEND_DESC& desc, RenderState state, BaseString& label)
{
    memset(&desc, 0, sizeof(desc));

    desc.AlphaToCoverageEnable = false;
    desc.IndependentBlendEnable = false;

    if (state.f.colorEnable)
    {
        desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

        switch (state.f.blendMode)
        {
            case BlendMode::Src:
            {
                desc.RenderTarget[0].BlendEnable = false;
                break;
            }
            case BlendMode::SrcOver:
            {
                label += "_SrcOver";
                desc.RenderTarget[0].BlendEnable = true;
                desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
                desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
                desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
                desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
                break;
            }
            case BlendMode::SrcOver_Multiply:
            {
                label += "_SrcOver_Multiply";
                desc.RenderTarget[0].BlendEnable = true;
                desc.RenderTarget[0].SrcBlend = D3D12_BLEND_DEST_COLOR;
                desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
                desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
                desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
                break;
            }
            case BlendMode::SrcOver_Screen:
            {
                label += "_SrcOver_Screen";
                desc.RenderTarget[0].BlendEnable = true;
                desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
                desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_COLOR;
                desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
                desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
                break;
            }
            case BlendMode::SrcOver_Additive:
            {
                label += "_SrcOver_Additive";
                desc.RenderTarget[0].BlendEnable = true;
                desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
                desc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
                desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
                desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
                break;
            }
            case BlendMode::SrcOver_Dual:
            {
                label += "_SrcOver_Dual";
                desc.RenderTarget[0].BlendEnable = true;
                desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
                desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC1_COLOR;
                desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
                desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC1_ALPHA;
                break;
            }
            default:
                NS_ASSERT_UNREACHABLE;
        }
    }
    else
    {
        desc.RenderTarget[0].BlendEnable = false;
        desc.RenderTarget[0].RenderTargetWriteMask = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void DepthStencilDesc(D3D12_DEPTH_STENCIL_DESC& desc, RenderState state, BaseString& label)
{
    memset(&desc, 0, sizeof(desc));

    desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.StencilReadMask = 0xFF;
    desc.StencilWriteMask = 0xFF;
    desc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    desc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    desc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;

    switch (state.f.stencilMode)
    {
        case StencilMode::Disabled:
        {
            desc.DepthEnable = false;
            desc.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
            desc.StencilEnable = false;
            desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
            desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
            desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            break;
        }
        case StencilMode::Equal_Keep:
        {
            label += "_Equal_Keep";
            desc.DepthEnable = false;
            desc.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
            desc.StencilEnable = true;
            desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
            desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
            desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            break;
        }
        case StencilMode::Equal_Incr:
        {
            label += "_Equal_Incr";
            desc.DepthEnable = false;
            desc.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
            desc.StencilEnable = true;
            desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
            desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
            desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
            desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
            break;
        }
        case StencilMode::Equal_Decr:
        {
            label += "_Equal_Decr";
            desc.DepthEnable = false;
            desc.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
            desc.StencilEnable = true;
            desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
            desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_DECR;
            desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
            desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_DECR;
            break;
        }
        case StencilMode::Clear:
        {
            label += "_Clear";
            desc.DepthEnable = false;
            desc.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
            desc.StencilEnable = true;
            desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_ZERO;
            desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_ZERO;
            break;
        }
        case StencilMode::Disabled_ZTest:
        {
            label += "_ZTest";
            desc.DepthEnable = true;
            desc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            desc.StencilEnable = false;
            desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
            desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
            desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            break;
        }
        case StencilMode::Equal_Keep_ZTest:
        {
            label += "_Equal_Keep_ZTest";
            desc.DepthEnable = true;
            desc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            desc.StencilEnable = true;
            desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
            desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
            desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
            break;
        }
        default:
            NS_ASSERT_UNREACHABLE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Texture> D3D12RenderDevice::WrapTexture(ID3D12Resource* obj, uint32_t width, uint32_t height,
    uint32_t levels, bool isInverted, bool hasAlpha, D3D12_RESOURCE_STATES state)
{
    Ptr<D3D12Texture> texture = *new D3D12Texture();

    // Defer resource creation until we know the device pointer
    texture->device = nullptr;
    texture->obj = obj;
    texture->width = width;
    texture->height = height;
    texture->levels = levels;
    texture->isInverted = isInverted;
    texture->hasAlpha = hasAlpha;
    texture->state = state;

    obj->AddRef();

    return texture;
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
static void FillElements(BaseVector<D3D12_INPUT_ELEMENT_DESC>& elements, uint32_t format)
{
    uint32_t attributes = AttributesForFormat[format];

    for (uint32_t i = 0; i < Shader::Vertex::Format::Attr::Count; i++)
    {
        if (attributes & (1 << i))
        {
            D3D12_INPUT_ELEMENT_DESC& element = elements.EmplaceBack();
            element.SemanticName = Semantic(i).first;
            element.SemanticIndex = Semantic(i).second;
            element.Format = Format(TypeForAttr[i]);
            element.InputSlot = 0;
            element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            element.InstanceDataStepRate = 0;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* D3D12RenderDevice::CreatePixelShader(const char* label, uint8_t shader, const void* hlsl,
    uint32_t size)
{
#ifdef NS_PROFILE
    uint64_t t0 = HighResTimer::Ticks();
#endif

    uint32_t signature;
    memcpy(&signature, hlsl, sizeof(signature));
    hlsl = (uint8_t*)hlsl + sizeof(signature);
    size -= sizeof(signature);

    uint8_t* shaders = (uint8_t*)Alloc(FastLZ::DecompressBufferSize(Shaders));
    FastLZ::Decompress(Shaders, sizeof(Shaders), shaders);

    Program& program = mCustomShaders.EmplaceBack();
    program.rootSignature = CreateRootSignature(signature);
    program.signature = signature;

    // TODO: Xbox is compiling from DXIL too. Note that this requires having DXIL
    // embedded in the vertex shaders (dxc invoked without -D__XBOX_STRIP_DXIL)
    // This is giving 'Runtime Recompilation Required' warnings

    uint8_t vsIndex = VertexForShader[shader];
    const ShaderVS& vs = ShadersVS(vsIndex, mCaps.linearRendering, false);

    Vector<D3D12_INPUT_ELEMENT_DESC, Shader::Vertex::Format::Attr::Count> elements;
    uint32_t format = FormatForVertex[vsIndex];
    FillElements(elements, format);
    NS_ASSERT(elements.IsSmall());

    program.stride = SizeForFormat[format];

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = program.rootSignature;
    desc.VS.pShaderBytecode = shaders + vs.start;
    desc.VS.BytecodeLength = vs.size;
    desc.PS.pShaderBytecode = hlsl;
    desc.PS.BytecodeLength = size;
    desc.InputLayout.pInputElementDescs = elements.Data();
    desc.InputLayout.NumElements = elements.Size();
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = mColorFormat;
    desc.DSVFormat = mStencilFormat;
    desc.SampleDesc = mSampleDesc;
    desc.SampleMask = 0xFFFFFFFF;

    CreatePipelines(&program, shader, label, desc, false);

  #if STEREO_RENDERING
    Program& programVR = mCustomShadersVR.EmplaceBack();
    const ShaderVS& vsVR = ShadersVS(vsIndex, mCaps.linearRendering, true);
    desc.VS.pShaderBytecode = shaders + vsVR.start;
    desc.VS.BytecodeLength = vsVR.size;

    programVR.signature = program.signature;
    programVR.rootSignature = program.rootSignature;
    programVR.stride = program.stride;

    CreatePipelines(&programVR, shader, label, desc, true);
  #endif

    Dealloc(shaders);

#ifdef NS_PROFILE
    uint64_t t1 = HighResTimer::Ticks();
    NS_LOG_TRACE("'%s' shader compiled in %.0f ms", label, 1000.0 * HighResTimer::Seconds(t1 - t0));
#endif

    return (void*)(uintptr_t)mCustomShaders.Size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::ClearPixelShaders()
{
  #if STEREO_RENDERING
    NS_ASSERT(mCustomShaders.Size() == mCustomShadersVR.Size());
    for (const Program& p : mCustomShadersVR)
    {
        for (uint32_t j = 0; j < NS_COUNTOF(Program::pso); j++)
        {
            DX_RELEASE(p.pso[j]);
        }
    }

    mCustomShadersVR.Clear();
  #endif

    for (const Program& p : mCustomShaders)
    {
        for (uint32_t j = 0; j < NS_COUNTOF(Program::pso); j++)
        {
            DX_RELEASE(p.pso[j]);
        }
    }

    mCustomShaders.Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::SetCommandList(ID3D12GraphicsCommandList* commands, uint64_t safeFenceValue)
{
    mCommands = commands;
    mSafeFenceValue = safeFenceValue;
    InvalidateStateCache();
    mPendingSplitBarriers.Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::SafeReleaseTexture(Texture* texture_)
{
    D3D12Texture* texture = (D3D12Texture*)texture_;
    PendingRelease p = { texture->srv, 0xFFFF, 0xFFFF, mSafeFenceValue, texture->obj };
    mPendingReleases.PushBack(p);
    mPendingSplitBarriers.EraseIf([&](Texture* _) { return _ == texture; });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::SafeReleaseRenderTarget(RenderTarget* surface_)
{
    D3D12RenderTarget* surface = (D3D12RenderTarget*)surface_;

    PendingRelease p0 = { 0xFFFF, surface->rtv, 0xFFFF, mSafeFenceValue, surface->colorAA };
    mPendingReleases.PushBack(p0);

    PendingRelease p1 = { 0xFFFF, 0xFFFF, surface->dsv, mSafeFenceValue, surface->stencil };
    mPendingReleases.PushBack(p1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::SafeReleaseResource(ID3D12Resource* resource)
{
    PendingRelease p = { 0xFFFF, 0xFFFF, 0xFFFF, mSafeFenceValue, resource };
    mPendingReleases.PushBack(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void EnsureShaderResourceState(ID3D12GraphicsCommandList* commands, D3D12Texture* texture)
{
    if (texture->state != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    {
        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.pResource = texture->obj;
        barrier.Transition.StateBefore = texture->state;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        if (texture->pendingBarrierToSRV)
        {
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
            texture->pendingBarrierToSRV = false;
        }

        commands->ResourceBarrier(1, &barrier);
        texture->state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::EndPendingSplitBarriers()
{
    for (Texture* texture : mPendingSplitBarriers)
    {
        EnsureShaderResourceState(mCommands, (D3D12Texture*)texture);
    }

    mPendingSplitBarriers.Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D12Resource* D3D12RenderDevice::GetNativePtr(Noesis::Texture* texture)
{
    return ((D3D12Texture*)texture)->obj;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D12_RESOURCE_STATES D3D12RenderDevice::GetTextureState(Texture* texture)
{
    return ((D3D12Texture*)texture)->state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::SetTextureState(Texture* texture, D3D12_RESOURCE_STATES state)
{
    ((D3D12Texture*)texture)->state = state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const DeviceCaps& D3D12RenderDevice::GetCaps() const
{
    return mCaps;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderTarget> D3D12RenderDevice::CreateRenderTarget(const char* label, uint32_t width,
    uint32_t height, uint32_t sampleCount, bool needsStencil)
{
    NS_UNUSED(sampleCount);

    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.SampleDesc = mSampleDesc;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    D3D12_HEAP_PROPERTIES heap;
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap.CreationNodeMask = 0;
    heap.VisibleNodeMask = 0;

    ID3D12Resource* colorAA = 0;

    if (mSampleDesc.Count > 1)
    {
        desc.Format = mColorFormat;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        V(mDevice->CreateCommittedResource(&heap, mHeapFlags, &desc,
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE, nullptr, DX_ARGS(&colorAA)));
        DX_NAME(colorAA, "Noesis_%s_AA", label);
    }

    ID3D12Resource* stencil = 0;

    if (needsStencil)
    {
        desc.Format = mStencilFormat;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        V(mDevice->CreateCommittedResource(&heap, mHeapFlags, &desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr, DX_ARGS(&stencil)));
        DX_NAME(stencil, "Noesis_%s_Stencil", label);
    }

    return CreateRenderTarget(label, width, height, colorAA, stencil);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderTarget> D3D12RenderDevice::CloneRenderTarget(const char* label, RenderTarget* surface_)
{
    D3D12RenderTarget* surface = (D3D12RenderTarget*)surface_;

    if (surface->colorAA)
    {
        surface->colorAA->AddRef();
    }

    if (surface->stencil)
    {
        surface->stencil->AddRef();
    }

    return CreateRenderTarget(label, surface->width, surface->height, surface->colorAA, surface->stencil);
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
Ptr<Texture> D3D12RenderDevice::CreateTexture(const char* label, uint32_t width, uint32_t height,
    uint32_t numLevels, TextureFormat::Enum format_, const void** data)
{
    DXGI_FORMAT format = DXGIFormat(format_, mCaps.linearRendering);

    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = (UINT16)numLevels;
    desc.Format = format;
    desc.SampleDesc = { 1, 0 };
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heap;
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap.CreationNodeMask = 0;
    heap.VisibleNodeMask = 0;

    ID3D12Resource* obj;
    V(mDevice->CreateCommittedResource(&heap, mHeapFlags, &desc, D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, DX_ARGS(&obj)));
    DX_NAME(obj, "Noesis_%s", label);

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

    if (data != nullptr)
    {
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprints[16];
        UINT numRows[16];
        UINT64 rowSizes[16];
        UINT64 totalBytes;

        NS_ASSERT(numLevels < NS_COUNTOF(footprints));
        mDevice->GetCopyableFootprints(&desc, 0, numLevels, 0, footprints, numRows, rowSizes, &totalBytes);

        heap.Type = D3D12_HEAP_TYPE_UPLOAD;
        heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap.CreationNodeMask = 0;
        heap.VisibleNodeMask = 0;

        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Width = totalBytes;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc = { 1, 0 };
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ID3D12Resource* staging;
        V(mDevice->CreateCommittedResource(&heap, mHeapFlags, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, DX_ARGS(&staging)));

        uint8_t* base;
        V(staging->Map(0, nullptr, (void**)&base));

        for (uint32_t level = 0; level < numLevels; level++)
        {
            uint64_t sourcePitch = rowSizes[level];
            const uint8_t* src = (const uint8_t*)data[level];
            uint8_t* dst = base + footprints[level].Offset;

            for (uint32_t i = 0; i < numRows[level]; i++)
            {
                memcpy(dst, src, (size_t)sourcePitch);
                src += sourcePitch;
                dst += footprints[level].Footprint.RowPitch;
            }

            D3D12_TEXTURE_COPY_LOCATION copyDest;
            copyDest.pResource = obj;
            copyDest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            copyDest.SubresourceIndex = level;

            D3D12_TEXTURE_COPY_LOCATION copySrc;
            copySrc.pResource = staging;
            copySrc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            copySrc.PlacedFootprint = footprints[level];

            mCommands->CopyTextureRegion(&copyDest, 0, 0, 0, &copySrc, nullptr);
        }

        SafeReleaseResource(staging);
    }

    NS_ASSERT(mSRVPool.nextDescriptor < MAX_SRV_DESCRIPTORS);
    uint32_t srv = AllocateDescriptor(mSRVPool);
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor = { mSRVHeapStartCPU + UINT64(srv) * mSRVSize };
    mDevice->CreateShaderResourceView(obj, 0, descriptor);

    Ptr<D3D12Texture> texture = *new D3D12Texture();
    texture->device = this;
    texture->obj = obj;
    texture->srv = srv;
    texture->width = width;
    texture->height = height;
    texture->levels = numLevels;
    texture->hasAlpha = format_ == TextureFormat::RGBA8;
    texture->state = D3D12_RESOURCE_STATE_COPY_DEST;

    return texture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static uint32_t Align(uint32_t n, uint32_t alignment)
{
    NS_ASSERT(IsPow2(alignment));
    return (n + (alignment - 1)) & ~(alignment - 1);
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
void D3D12RenderDevice::UpdateTexture(Texture* texture_, uint32_t level, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height, const void* data)
{
    D3D12Texture* texture = (D3D12Texture*)texture_;
    D3D12_RESOURCE_DESC desc = texture->obj->GetDesc();
    uint32_t texelBytes = TexelBytes(desc.Format);

    if (texture->state != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = texture->obj;
        barrier.Transition.Subresource = 0;
        barrier.Transition.StateBefore = texture->state;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        mCommands->ResourceBarrier(1, &barrier);

        texture->state = D3D12_RESOURCE_STATE_COPY_DEST;
    }

    // If the texture is bound (as DATA_STATIC_WHILE_SET_AT_EXECUTE) it is required to be rebound
    for (uint32_t i = 0; i < sizeof(mCachedRootSignatureParameters); i++)
    {
        UINT64 addr = mSRVHeapStartGPU + mSRVSize * texture->srv;
        if (mCachedRootSignatureParameters[i] == addr)
        {
            RebindRootSignatureDescriptors();
            break;
        }
    }

    D3D12_SUBRESOURCE_FOOTPRINT footprint;
    footprint.Format = desc.Format;
    footprint.Width = width;
    footprint.Height = height;
    footprint.Depth = 1;
    footprint.RowPitch = Align(width * texelBytes, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

    uint32_t bytes = Align(height * footprint.RowPitch, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    void* ptr = MapBuffer(mTexUpload, bytes);

    {
        uint32_t sourcePitch = width * texelBytes;
        const uint8_t* src = (const uint8_t*)data;
        uint8_t* dst = (uint8_t*)ptr;

        for (uint32_t i = 0; i < height; i++)
        {
            memcpy(dst, src, sourcePitch);
            src += sourcePitch;
            dst += footprint.RowPitch;
        }
    }

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedFootprint;
    placedFootprint.Offset = mTexUpload.drawPos;
    placedFootprint.Footprint = footprint;

    D3D12_TEXTURE_COPY_LOCATION dst;
    dst.pResource = texture->obj;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = level;

    D3D12_TEXTURE_COPY_LOCATION src;
    src.pResource = mTexUpload.currentPage->buffer;
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = placedFootprint;

    mCommands->CopyTextureRegion(&dst, x, y, 0, &src, nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::BeginOffscreenRender()
{
    DX_BEGIN_EVENT("Noesis.Offscreen");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::EndOffscreenRender()
{
    DX_END_EVENT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::BeginOnscreenRender()
{
    ProcessPendingReleases(false);
    DX_BEGIN_EVENT("Noesis");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::EndOnscreenRender()
{
    DX_END_EVENT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::SetRenderTarget(RenderTarget* surface_)
{
    D3D12RenderTarget* surface = (D3D12RenderTarget*)surface_;
    DX_BEGIN_EVENT("SetRenderTarget");

    if (surface->colorAA)
    {
        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.Subresource = 0;
        barrier.Transition.pResource = surface->colorAA;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        mCommands->ResourceBarrier(1, &barrier);
    }
    else
    {
        EnsureShaderResourceState(mCommands, surface->texture);

        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.Subresource = 0;
        barrier.Transition.pResource = surface->texture->obj;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        mCommands->ResourceBarrier(1, &barrier);
        surface->texture->state = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtDesc = { mRTVHeapStart + mRTVSize * surface->rtv };
    D3D12_CPU_DESCRIPTOR_HANDLE stencilDesc = { mDSVHeapStart + mDSVSize * surface->dsv };
    bool hasStencil = surface->dsv != 0xFFFF;
    mCommands->OMSetRenderTargets(1, &rtDesc, false, hasStencil ? &stencilDesc : nullptr);

    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = (FLOAT)surface->width;
    viewport.Height = (FLOAT)surface->height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    mCommands->RSSetViewports(1, &viewport);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::BeginTile(RenderTarget* surface_, const Tile& tile)
{
    D3D12RenderTarget* surface = (D3D12RenderTarget*)surface_;

    D3D12_RECT rect;
    rect.left = tile.x;
    rect.top = surface->height - (tile.y + tile.height);
    rect.right = tile.x + tile.width;
    rect.bottom = surface->height - tile.y;

    mCommands->RSSetScissorRects(1, &rect);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::EndTile(RenderTarget*)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::ResolveRenderTarget(RenderTarget* surface_, const Tile* tiles, uint32_t size)
{
    D3D12RenderTarget* surface = (D3D12RenderTarget*)surface_;

    if (surface->colorAA)
    {
        DX_BEGIN_EVENT("Resolve");

        EnsureShaderResourceState(mCommands, surface->texture);
        mCommands->DiscardResource(surface->colorAA, 0);

        D3D12_RESOURCE_BARRIER barriers[2];
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[0].Transition.Subresource = 0;
        barriers[0].Transition.pResource = surface->colorAA;
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[1].Transition.Subresource = 0;
        barriers[1].Transition.pResource = surface->texture->obj;
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_DEST;

        mCommands->ResourceBarrier(2, barriers);
        surface->texture->state = D3D12_RESOURCE_STATE_RESOLVE_DEST;

        ID3D12GraphicsCommandList1* commands1;
        V(mCommands->QueryInterface(DX_ARGS(&commands1)));

        for (uint32_t i = 0; i < size; i++)
        {
            const Tile& tile = tiles[i];

            UINT dstX = tile.x;
            UINT dstY = surface->height - (tile.y + tile.height);

            D3D12_RECT src;
            src.left = dstX;
            src.top = dstY;
            src.right = dstX + tile.width;
            src.bottom = dstY + tile.height;

            commands1->ResolveSubresourceRegion(surface->texture->obj, 0, dstX, dstY,
                surface->colorAA, 0, &src, mColorFormat, D3D12_RESOLVE_MODE_AVERAGE);
        }

        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
        barrier.Transition.Subresource = 0;
        barrier.Transition.pResource = surface->texture->obj;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        mCommands->ResourceBarrier(1, &barrier);
        surface->texture->pendingBarrierToSRV = true;
        mPendingSplitBarriers.PushBack(surface->texture);

        DX_RELEASE(commands1);
        DX_END_EVENT();
    }
    else
    {
        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
        barrier.Transition.Subresource = 0;
        barrier.Transition.pResource = surface->texture->obj;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter =  D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        mCommands->ResourceBarrier(1, &barrier);
        surface->texture->pendingBarrierToSRV = true;
        mPendingSplitBarriers.PushBack(surface->texture);
    }

    if (surface->stencil)
    {
        mCommands->DiscardResource(surface->stencil, 0);
    }

    DX_END_EVENT();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* D3D12RenderDevice::MapVertices(uint32_t bytes)
{
    return MapBuffer(mVertices, bytes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::UnmapVertices()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* D3D12RenderDevice::MapIndices(uint32_t bytes)
{
    return MapBuffer(mIndices, bytes);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::UnmapIndices()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static uint32_t GetSignature(const Batch& batch)
{
    uint32_t signature = 0;

    if (batch.pattern) signature |= PS_T0;
    if (batch.ramps) signature |= PS_T1;
    if (batch.image) signature |= PS_T2;
    if (batch.glyphs) signature |= PS_T3;
    if (batch.shadow) signature |= PS_T4;
    if (batch.vertexUniforms[0].values) signature |= VS_CB0;
    if (batch.vertexUniforms[1].values) signature |= VS_CB1;
    if (batch.pixelUniforms[0].values) signature |= PS_CB0;
    if (batch.pixelUniforms[1].values) signature |= PS_CB1;

    return signature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::DrawBatch(const Batch& batch)
{
    Program* program;

    if (batch.pixelShader != nullptr)
    {
      #if STEREO_RENDERING
        if (batch.singlePassStereo)
        {
            NS_ASSERT((uintptr_t)batch.pixelShader <= mCustomShadersVR.Size());
            program = &mCustomShadersVR[(int)(uintptr_t)batch.pixelShader - 1];
        }
        else
      #endif
        {
            NS_ASSERT((uintptr_t)batch.pixelShader <= mCustomShaders.Size());
            program = &mCustomShaders[(int)(uintptr_t)batch.pixelShader - 1];
        }
    }
    else
    {
      #if STEREO_RENDERING
        if (batch.singlePassStereo)
        {
            NS_ASSERT(batch.shader.v < NS_COUNTOF(mProgramsVR));
            NS_ASSERT(mProgramsVR[batch.shader.v].rootSignature != nullptr);
            program = &mProgramsVR[batch.shader.v];
        }
        else
      #endif
        {
            NS_ASSERT(batch.shader.v < NS_COUNTOF(mPrograms));
            NS_ASSERT(mPrograms[batch.shader.v].rootSignature != nullptr);
            program = &mPrograms[batch.shader.v];
        }
    }

    // Skip draw if shader requires something not available in the batch
    if (NS_UNLIKELY((program->signature & GetSignature(batch)) != program->signature)) return;

    SetBuffers(batch, program->stride);
    SetRootSignature(program->rootSignature);
    SetDescriptors(batch, program->signature);
    SetPipelineState(program->pso[batch.renderState.v]);
    SetStencilRef(batch.stencilRef);

    UINT startIndexLocation = batch.startIndex + mIndices.drawPos / 2;
    UINT instanceCount = batch.singlePassStereo ? 2 : 1;
    mCommands->DrawIndexedInstanced(batch.numIndices, instanceCount, startIndexLocation, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::ProcessPendingReleases(bool force)
{
    if (!mPendingReleases.Empty())
    {
        PendingRelease* it = mPendingReleases.Begin();
        while (it != mPendingReleases.End())
        {
            if (mFrameFence->GetCompletedValue() >= it->safeFenceValue || force)
            {
                if (it->srv != 0xFFFF)
                {
                    NS_LOG_TRACE("=> Deallocate SRV (%d)", it->srv);
                    FreeDescriptor(mSRVPool, it->srv);
                }

                if (it->rtv != 0xFFFF)
                {
                    NS_LOG_TRACE("=> Deallocate RTV (%d)", it->rtv);
                    FreeDescriptor(mRTVPool, it->rtv);
                }

                if (it->dsv != 0xFFFF)
                {
                    NS_LOG_TRACE("=> Deallocate DSV (%d)", it->dsv);
                    FreeDescriptor(mDSVPool, it->dsv);
                }

                if (it->object)
                {
                    NS_LOG_TRACE("=> Release (%p)", it->object);
                    it->object->Release();
                }

                it++;
            }
            else
            {
                break;
            }
        }

        mPendingReleases.Erase(mPendingReleases.Begin(), it);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t D3D12RenderDevice::AllocateDescriptor(DescriptorPool& pool)
{
    if (!pool.freeDescriptors.Empty())
    {
        uint16_t index = pool.freeDescriptors.Back();
        pool.freeDescriptors.PopBack();
        return index;
    }

    return pool.nextDescriptor++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::FreeDescriptor(DescriptorPool& pool, uint32_t descriptor)
{
    pool.freeDescriptors.PushBack((uint16_t)descriptor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D12RenderDevice::Page* D3D12RenderDevice::AllocatePage(PageAllocator& allocator)
{
    using Block = PageAllocator::Block;

    if (!allocator.blocks || allocator.blocks->count == NS_COUNTOF(Block::pages))
    {
        Block* block = (Block*)Noesis::Alloc(sizeof(Block));
        block->count = 0;
        block->next = allocator.blocks;
        allocator.blocks = block;
    }

    return allocator.blocks->pages + allocator.blocks->count++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D12RenderDevice::Page* D3D12RenderDevice::AllocatePage(DynamicBuffer& buffer)
{
    Page* page = AllocatePage(buffer.allocator);
    memset(page, 0, sizeof(Page));

    D3D12_HEAP_PROPERTIES heap;
    heap.Type = D3D12_HEAP_TYPE_UPLOAD;
    heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap.CreationNodeMask = 0;
    heap.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = buffer.size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc = { 1, 0 };
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    V(mDevice->CreateCommittedResource(&heap, mHeapFlags, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, DX_ARGS(&page->buffer)));
    V(page->buffer->Map(0, nullptr, &page->base));

    DX_NAME(page->buffer, "Noesis_%s[%d]", buffer.label, buffer.numPages);
    NS_LOG_TRACE("Page '%s[%d]' created (%d KB)", buffer.label, buffer.numPages, buffer.size / 1024);

    page->safeFenceValue = 0;
    page->gpuAddress = page->buffer->GetGPUVirtualAddress();

    buffer.numPages++;
    return page;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::CreateBuffer(DynamicBuffer& buffer, uint32_t size, const char* label)
{
    buffer.size = size;
    buffer.pos = 0;
    buffer.drawPos = 0;
    buffer.label = label;

    buffer.numPages = 0;
    buffer.pendingPages = nullptr;
    buffer.freePages = nullptr;

    for (uint32_t i = 0; i < PREALLOCATED_DYNAMIC_PAGES; i++)
    {
        Page* page = AllocatePage(buffer);
        page->next = buffer.freePages;
        buffer.freePages = page;
    }

    NS_ASSERT(buffer.freePages != nullptr);
    buffer.currentPage = buffer.freePages;
    buffer.freePages = buffer.freePages->next;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::CreateBuffers()
{
    memset(mCachedConstantHash, 0, sizeof(mCachedConstantHash));
    memset(mCachedConstantAddress, 0, sizeof(mCachedConstantAddress));

    CreateBuffer(mVertices, DYNAMIC_VB_SIZE, "Vertices");
    CreateBuffer(mIndices, DYNAMIC_IB_SIZE, "Indices");
    CreateBuffer(mConstants[0], CBUFFER_SIZE, "VertexCB0");
    CreateBuffer(mConstants[1], CBUFFER_SIZE, "VertexCB1");
    CreateBuffer(mConstants[2], CBUFFER_SIZE, "PixelCB0");
    CreateBuffer(mConstants[3], CBUFFER_SIZE, "PixelCB1");
    CreateBuffer(mTexUpload, DYNAMIC_TEX_SIZE, "TexUpload");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::DestroyBuffer(DynamicBuffer& buffer)
{
    PageAllocator::Block* block = buffer.allocator.blocks;

    while (block)
    {
        for (uint32_t i = 0; i < block->count; i++)
        {
            DX_DESTROY(block->pages[i].buffer);
        }

        void* ptr = block;
        block = block->next;
        Noesis::Dealloc(ptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::DestroyBuffers()
{
    DestroyBuffer(mVertices);
    DestroyBuffer(mIndices);
    DestroyBuffer(mTexUpload);

    for (uint32_t i = 0; i < NS_COUNTOF(mConstants); i++)
    {
        DestroyBuffer(mConstants[i]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* D3D12RenderDevice::MapBuffer(DynamicBuffer& buffer, uint32_t size)
{
    NS_ASSERT(size <= buffer.size);

    if (buffer.pos + size > buffer.size)
    {
        // We ran out of space in the current page, get a new one
        // Move the current one to pending and insert a GPU fence
        buffer.currentPage->safeFenceValue = mSafeFenceValue;
        buffer.currentPage->next = buffer.pendingPages;
        buffer.pendingPages = buffer.currentPage;

        // If there is one free slot get it
        if (buffer.freePages != nullptr)
        {
            buffer.currentPage = buffer.freePages;
            buffer.freePages = buffer.freePages->next;
        }
        else
        {
            // Move pages already processed by GPU from pending to free
            Page** it = &buffer.pendingPages->next;
            while (*it != nullptr)
            {
                if (mFrameFence->GetCompletedValue() < (*it)->safeFenceValue)
                {
                    it = &((*it)->next);
                }
                else
                {
                    // Once we find a processed page, the rest of pages must be also processed
                    Page* page = *it;
                    *it = nullptr;

                    NS_ASSERT(buffer.freePages == nullptr);
                    buffer.freePages = page;
                    break;
                }
            }

            if (buffer.freePages != nullptr)
            {
                buffer.currentPage = buffer.freePages;
                buffer.freePages = buffer.freePages->next;
            }
            else
            {
                buffer.currentPage = AllocatePage(buffer);
            }
        }

        buffer.pos = 0;
    }

    buffer.drawPos = buffer.pos;
    buffer.pos = buffer.pos + size;
    return (uint8_t*)buffer.currentPage->base + buffer.drawPos;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void SetFilter(MinMagFilter::Enum minmag, MipFilter::Enum mip, D3D12_SAMPLER_DESC& desc)
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
                    desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
                    break;
                }
                case MipFilter::Nearest:
                {
                    desc.MaxLOD = D3D12_FLOAT32_MAX;
                    desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
                    break;
                }
                case MipFilter::Linear:
                {
                    desc.MaxLOD = D3D12_FLOAT32_MAX;
                    desc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
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
                    desc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                    break;
                }
                case MipFilter::Nearest:
                {
                    desc.MaxLOD = D3D12_FLOAT32_MAX;
                    desc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                    break;
                }
                case MipFilter::Linear:
                {
                    desc.MaxLOD = D3D12_FLOAT32_MAX;
                    desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
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
static void SetAddress(WrapMode::Enum mode, D3D12_SAMPLER_DESC& desc)
{
    switch (mode)
    {
        case WrapMode::ClampToEdge:
        {
            desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            break;
        }
        case WrapMode::ClampToZero:
        {
            desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            break;
        }
        case WrapMode::Repeat:
        {
            desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            break;
        }
        case WrapMode::MirrorU:
        {
            desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
            desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            break;
        }
        case WrapMode::MirrorV:
        {
            desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
            break;
        }
        case WrapMode::Mirror:
        {
            desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
            desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
            break;
        }
        default:
        {
            NS_ASSERT_UNREACHABLE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static D3D12_ROOT_PARAMETER CBVRootParameter(UINT reg, D3D12_SHADER_VISIBILITY visibility)
{
    D3D12_ROOT_PARAMETER param;
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.Descriptor.ShaderRegister = reg;
    param.Descriptor.RegisterSpace = 0;
    param.ShaderVisibility = visibility;
    return param;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static D3D12_ROOT_PARAMETER1 CBVRootParameter1(UINT reg, D3D12_SHADER_VISIBILITY visibility)
{
    D3D12_ROOT_PARAMETER1 param;
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.Descriptor.ShaderRegister = reg;
    param.Descriptor.RegisterSpace = 0;
    param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
    param.ShaderVisibility = visibility;
    return param;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static D3D12_ROOT_PARAMETER TableRootParameter(const D3D12_DESCRIPTOR_RANGE *range)
{
    D3D12_ROOT_PARAMETER param;
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = range;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    return param;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static D3D12_ROOT_PARAMETER1 TableRootParameter1(const D3D12_DESCRIPTOR_RANGE1 *range)
{
    D3D12_ROOT_PARAMETER1 param;
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = range;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    return param;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static D3D12_DESCRIPTOR_RANGE SRVRange(UINT reg)
{
    D3D12_DESCRIPTOR_RANGE range;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = reg;
    range.RegisterSpace = 0;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    return range;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static D3D12_DESCRIPTOR_RANGE1 SRVRange1(UINT reg)
{
    D3D12_DESCRIPTOR_RANGE1 range;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = reg;
    range.RegisterSpace = 0;
    range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    return range;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static D3D12_DESCRIPTOR_RANGE SamplerRange(UINT reg)
{
    D3D12_DESCRIPTOR_RANGE range;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = reg;
    range.RegisterSpace = 0;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    return range;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static D3D12_DESCRIPTOR_RANGE1 SamplerRange1(UINT reg)
{
    D3D12_DESCRIPTOR_RANGE1 range;
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = reg;
    range.RegisterSpace = 0;
    range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    return range;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D12RootSignature* D3D12RenderDevice::CreateRootSignature(uint32_t flags)
{
    RootSignatures::InsertResult it =  mRootSignatures.Insert(flags);
    if (!it.second)
    {
        return it.first->value;
    }

    D3D12_FEATURE_DATA_ROOT_SIGNATURE data = { D3D_ROOT_SIGNATURE_VERSION_1_1 };

    if (D3D12SerializeVersionedRootSignature == 0)
    {
        data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    if (FAILED(mDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &data, sizeof(data))))
    {
        data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    FixedString<256> label;
    ID3DBlob* blob = 0;

    if (data.HighestVersion == D3D_ROOT_SIGNATURE_VERSION_1_0)
    {
        D3D12_DESCRIPTOR_RANGE ranges[] =
        {
            SRVRange(0), SamplerRange(0),
            SRVRange(1), SamplerRange(1),
            SRVRange(2), SamplerRange(2),
            SRVRange(3), SamplerRange(3),
            SRVRange(4), SamplerRange(4)
        };

        uint32_t i = 0;
        D3D12_ROOT_PARAMETER rootParameters[10];

        if (flags & VS_CB0)
        {
            label += "VSb0";
            rootParameters[i++] = CBVRootParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
        }

        if (flags & VS_CB1)
        {
            label += "_VSb1";
            rootParameters[i++] = CBVRootParameter(1, D3D12_SHADER_VISIBILITY_VERTEX);
        }

        if (flags & PS_CB0)
        {
            label += "_PSb0";
            rootParameters[i++] = CBVRootParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
        }

        if (flags & PS_CB1)
        {
            label += "_PSb1";
            rootParameters[i++] = CBVRootParameter(1, D3D12_SHADER_VISIBILITY_PIXEL);
        }

        if (flags & PS_T0)
        {
            label += "_T0";
            rootParameters[i++] = TableRootParameter(&ranges[0]);
            rootParameters[i++] = TableRootParameter(&ranges[1]);
        }

        if (flags & PS_T1)
        {
            label += "_T1";
            rootParameters[i++] = TableRootParameter(&ranges[2]);
            rootParameters[i++] = TableRootParameter(&ranges[3]);
        }

        if (flags & PS_T2)
        {
            label += "_T2";
            rootParameters[i++] = TableRootParameter(&ranges[4]);
            rootParameters[i++] = TableRootParameter(&ranges[5]);
        }

        if (flags & PS_T3)
        {
            label += "_T3";
            rootParameters[i++] = TableRootParameter(&ranges[6]);
            rootParameters[i++] = TableRootParameter(&ranges[7]);
        }

        if (flags & PS_T4)
        {
            label += "_T4";
            rootParameters[i++] = TableRootParameter(&ranges[8]);
            rootParameters[i++] = TableRootParameter(&ranges[9]);
        }

        NS_ASSERT(i <= NS_COUNTOF(rootParameters));

        D3D12_ROOT_SIGNATURE_DESC desc;
        desc.NumParameters = i;
        desc.pParameters = rootParameters;
        desc.NumStaticSamplers = 0;
        desc.pStaticSamplers = nullptr;
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        V(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, nullptr));
    }
    else
    {
        D3D12_DESCRIPTOR_RANGE1 ranges[] =
        {
            SRVRange1(0), SamplerRange1(0),
            SRVRange1(1), SamplerRange1(1),
            SRVRange1(2), SamplerRange1(2),
            SRVRange1(3), SamplerRange1(3),
            SRVRange1(4), SamplerRange1(4)
        };

        uint32_t i = 0;
        D3D12_ROOT_PARAMETER1 rootParameters[10];

        if (flags & VS_CB0)
        {
            label += "VSb0";
            rootParameters[i++] = CBVRootParameter1(0, D3D12_SHADER_VISIBILITY_VERTEX);
        }

        if (flags & VS_CB1)
        {
            label += "_VSb1";
            rootParameters[i++] = CBVRootParameter1(1, D3D12_SHADER_VISIBILITY_VERTEX);
        }

        if (flags & PS_CB0)
        {
            label += "_PSb0";
            rootParameters[i++] = CBVRootParameter1(0, D3D12_SHADER_VISIBILITY_PIXEL);
        }

        if (flags & PS_CB1)
        {
            label += "_PSb1";
            rootParameters[i++] = CBVRootParameter1(1, D3D12_SHADER_VISIBILITY_PIXEL);
        }

        if (flags & PS_T0)
        {
            label += "_T0";
            rootParameters[i++] = TableRootParameter1(&ranges[0]);
            rootParameters[i++] = TableRootParameter1(&ranges[1]);
        }

        if (flags & PS_T1)
        {
            label += "_T1";
            rootParameters[i++] = TableRootParameter1(&ranges[2]);
            rootParameters[i++] = TableRootParameter1(&ranges[3]);
        }

        if (flags & PS_T2)
        {
            label += "_T2";
            rootParameters[i++] = TableRootParameter1(&ranges[4]);
            rootParameters[i++] = TableRootParameter1(&ranges[5]);
        }

        if (flags & PS_T3)
        {
            label += "_T3";
            rootParameters[i++] = TableRootParameter1(&ranges[6]);
            rootParameters[i++] = TableRootParameter1(&ranges[7]);
        }

        if (flags & PS_T4)
        {
            label += "_T4";
            rootParameters[i++] = TableRootParameter1(&ranges[8]);
            rootParameters[i++] = TableRootParameter1(&ranges[9]);
        }

        NS_ASSERT(i <= NS_COUNTOF(rootParameters));

        D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        desc.Desc_1_1.NumParameters = i;
        desc.Desc_1_1.pParameters = rootParameters;
        desc.Desc_1_1.NumStaticSamplers = 0;
        desc.Desc_1_1.pStaticSamplers = nullptr;
        desc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        V(D3D12SerializeVersionedRootSignature(&desc, &blob, nullptr));
    }

    const void* blobSignature = blob->GetBufferPointer();
    SIZE_T blobSize = blob->GetBufferSize();
    V(mDevice->CreateRootSignature(0, blobSignature, blobSize, DX_ARGS(&it.first->value)));
    DX_NAME(it.first->value, "Noesis_%s", label.Str());
    DX_DESTROY(blob);

    return it.first->value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::CreateHeaps()
{
    // RTV heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = MAX_RTV_DESCRIPTORS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 0;

        V(mDevice->CreateDescriptorHeap(&desc, DX_ARGS(&mRTVHeap)));
        DX_NAME(mRTVHeap, "Noesis_RTV_Heap");

        mRTVHeapStart = mRTVHeap->GetCPUDescriptorHandleForHeapStart().ptr;
        mRTVSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // DSV heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        desc.NumDescriptors = MAX_DSV_DESCRIPTORS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 0;

        V(mDevice->CreateDescriptorHeap(&desc, DX_ARGS(&mDSVHeap)));
        DX_NAME(mDSVHeap, "Noesis_DSV_Heap");

        mDSVHeapStart = mDSVHeap->GetCPUDescriptorHandleForHeapStart().ptr;
        mDSVSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    // SRV heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = MAX_SRV_DESCRIPTORS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask = 0;

        V(mDevice->CreateDescriptorHeap(&desc, DX_ARGS(&mSRVHeap)));
        DX_NAME(mSRVHeap, "Noesis_SRV_Heap");

        mSRVHeapStartCPU = mSRVHeap->GetCPUDescriptorHandleForHeapStart().ptr;
        mSRVHeapStartGPU = mSRVHeap->GetGPUDescriptorHandleForHeapStart().ptr;
        mSRVSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    // Sampler heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        desc.NumDescriptors = 64;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask = 0;

        V(mDevice->CreateDescriptorHeap(&desc, DX_ARGS(&mSamplerHeap)));
        DX_NAME(mSamplerHeap, "Noesis_Sampler_Heap");

        mSamplerHeapStartGPU = mSamplerHeap->GetGPUDescriptorHandleForHeapStart().ptr;
        mSamplerSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }

    // Sampler states
    {
        D3D12_SAMPLER_DESC desc{};
        desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        desc.MipLODBias = -0.75f;
        desc.MaxAnisotropy = 1;
        desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        desc.MinLOD = -D3D12_FLOAT32_MAX;

        D3D12_CPU_DESCRIPTOR_HANDLE base = mSamplerHeap->GetCPUDescriptorHandleForHeapStart();
        UINT size = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        for (uint8_t minmag = 0; minmag < MinMagFilter::Count; minmag++)
        {
            for (uint8_t mip = 0; mip < MipFilter::Count; mip++)
            {
                SetFilter(MinMagFilter::Enum(minmag), MipFilter::Enum(mip), desc);

                for (uint8_t uv = 0; uv < WrapMode::Count; uv++)
                {
                    SetAddress(WrapMode::Enum(uv), desc);

                    SamplerState s = {{uv, minmag, mip}};
                    mDevice->CreateSampler(&desc, { base.ptr + (SIZE_T)s.v * size });
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::CreatePipelines(Program* program, uint8_t shader_, const char* label_,
    D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, bool stereo)
{
    memset(program->pso, 0, sizeof(program->pso));

#ifdef NS_PLATFORM_GAME_CORE
    NS_UNUSED(stereo);

    ID3D12PipelineState* src;
    desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    V(mDevice->CreateGraphicsPipelineState(&desc, DX_ARGS(&src)));

    for (uint32_t i = 0; i < 256; i++)
    {
        Shader shader;
        shader.v = shader_;

        RenderState state;
        state.v = (uint8_t)i;

        FixedString<256> label = label_;

        if (IsValidState(shader, state))
        {
            D3D12XBOX_DERIVED_GRAPHICS_STATE_DESC derived[3];
            derived[0].Type = D3D12XBOX_DERIVED_GRAPHICS_STATE_TYPE_BLEND;
            derived[1].Type = D3D12XBOX_DERIVED_GRAPHICS_STATE_TYPE_RASTERIZER;
            derived[2].Type = D3D12XBOX_DERIVED_GRAPHICS_STATE_TYPE_DEPTH_STENCIL;

            BlendDesc(derived[0].BlendDesc, state, label);
            RasterizerDesc(derived[1].RasterizerDesc, state, label);
            DepthStencilDesc(derived[2].DepthStencilDesc, state, label);

            V(mDevice->CreateDerivedGraphicsPipelineStateX(src, 3, derived, DX_ARGS(&program->pso[i])));
            DX_NAME(program->pso[i], "Noesis_%s", label.Str());
        }
    }

    src->Release();
#else
    ID3D12InfoQueue* infoQueue = nullptr;
    if (SUCCEEDED(mDevice->QueryInterface(IID_PPV_ARGS(&infoQueue))))
    {
        D3D12_MESSAGE_ID messages[] =
        {
            D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_RENDERTARGETVIEW_NOT_SET,
            D3D12_MESSAGE_ID_LOADPIPELINE_NAMENOTFOUND,
        };

        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = NS_COUNTOF(messages);
        filter.DenyList.pIDList = messages;
        infoQueue->PushCopyOfStorageFilter();
        infoQueue->AddStorageFilterEntries(&filter);
    }

    for (uint32_t i = 0; i < 256; i++)
    {
        Shader shader;
        shader.v = shader_;

        RenderState state;
        state.v = (uint8_t)i;

        ID3D12PipelineState** pso = &program->pso[i];
        FixedString<256> label = label_;

        if (IsValidState(shader, state))
        {
            BlendDesc(desc.BlendState, state, label);
            RasterizerDesc(desc.RasterizerState, state, label);
            DepthStencilDesc(desc.DepthStencilState, state, label);

            if (stereo)
            {
                label += "_Stereo";
            }

          #ifdef NS_PLATFORM_WINDOWS_DESKTOP
            // Read pipeline from cache
            uint32_t h0 = HashBytes(desc.PS.pShaderBytecode, (uint32_t)desc.PS.BytecodeLength);
            uint32_t h1 = HashCombine((int)mColorFormat, (int)mStencilFormat, mSampleDesc.Count);
            uint32_t h2 = HashCombine((uint32_t)state.v, (uint32_t)stereo);

            WCHAR name[MAX_PATH];
            _snwprintf_s(name, MAX_PATH, L"%08x_%08x_%08x_V2", h0, h1, h2);

            if (mLibrary)
            {
                if (SUCCEEDED(mLibrary->LoadGraphicsPipeline(name, &desc, DX_ARGS(pso))))
                {
                    continue;
                }
            }
          #endif

            V(mDevice->CreateGraphicsPipelineState(&desc, DX_ARGS(pso)));
            DX_NAME(*pso, "Noesis_%s", label.Str());

          #ifdef NS_PLATFORM_WINDOWS_DESKTOP
            if (mLibrary)
            {
                HRESULT hr = mLibrary->StorePipeline(name, *pso);

                if (FAILED(hr))
                {
                    // This failure indicates potential corruption in the pipeline library.
                    // Just start fresh by creating a new one.
                    DX_DESTROY(mLibrary);

                    ID3D12Device1* device1 = nullptr;
                    V(mDevice->QueryInterface(DX_ARGS(&device1)));
                    NS_LOG_WARNING("Regenerating Pipeline library cache");
                    device1->CreatePipelineLibrary(nullptr, 0, DX_ARGS(&mLibrary));
                    DX_RELEASE(device1);

                    V(mLibrary->StorePipeline(name, *pso));
                }

                mLibraryUpdated = true;
            }
          #endif
        }
    }

  #ifdef NS_DEBUG
    if (infoQueue)
    {
        infoQueue->PopStorageFilter();
        DX_RELEASE(infoQueue);
    }
  #endif
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::CreateShaders()
{
#ifdef NS_PROFILE
    uint64_t t0 = HighResTimer::Ticks();
#endif

    uint8_t* shaders = (uint8_t*)Alloc(FastLZ::DecompressBufferSize(Shaders));
    FastLZ::Decompress(Shaders, sizeof(Shaders), shaders);

#define PSHADER(n, rs) case Shader::n: return ShaderPS { #n, n##_PS_Start, n##_PS_Size, rs };

    struct ShaderPS
    {
        const char* label;
        uint32_t start;
        uint32_t size;
        uint32_t signature;
    };

    auto pShaders = [](uint32_t shader)
    {
        switch (shader)
        {
            PSHADER(RGBA, VS_CB0 | PS_CB0)
            PSHADER(Mask, VS_CB0)
            PSHADER(Clear, VS_CB0)

            PSHADER(Path_Solid, VS_CB0)
            PSHADER(Path_Linear, VS_CB0 | PS_CB0 | PS_T1)
            PSHADER(Path_Radial, VS_CB0 | PS_CB0 | PS_T1)
            PSHADER(Path_Pattern, VS_CB0 | PS_CB0 | PS_T0)
            PSHADER(Path_Pattern_Clamp, VS_CB0 | PS_CB0 | PS_T0)
            PSHADER(Path_Pattern_Repeat, VS_CB0 | PS_CB0 | PS_T0)
            PSHADER(Path_Pattern_MirrorU, VS_CB0 | PS_CB0 | PS_T0)
            PSHADER(Path_Pattern_MirrorV, VS_CB0 | PS_CB0 | PS_T0)
            PSHADER(Path_Pattern_Mirror, VS_CB0 | PS_CB0 | PS_T0)

            PSHADER(Path_AA_Solid, VS_CB0)
            PSHADER(Path_AA_Linear, VS_CB0 | PS_CB0 | PS_T1)
            PSHADER(Path_AA_Radial, VS_CB0 | PS_CB0 | PS_T1)
            PSHADER(Path_AA_Pattern, VS_CB0 | PS_CB0 | PS_T0)
            PSHADER(Path_AA_Pattern_Clamp, VS_CB0 | PS_CB0 | PS_T0)
            PSHADER(Path_AA_Pattern_Repeat, VS_CB0 | PS_CB0 | PS_T0)
            PSHADER(Path_AA_Pattern_MirrorU, VS_CB0 | PS_CB0 | PS_T0)
            PSHADER(Path_AA_Pattern_MirrorV, VS_CB0 | PS_CB0 | PS_T0)
            PSHADER(Path_AA_Pattern_Mirror, VS_CB0 | PS_CB0 | PS_T0)

            PSHADER(SDF_Solid, VS_CB0 | VS_CB1 | PS_T3)
            PSHADER(SDF_Linear, VS_CB0 | VS_CB1 | PS_CB0 | PS_T1 | PS_T3)
            PSHADER(SDF_Radial, VS_CB0 | VS_CB1 | PS_CB0 | PS_T1 | PS_T3)
            PSHADER(SDF_Pattern, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
            PSHADER(SDF_Pattern_Clamp, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
            PSHADER(SDF_Pattern_Repeat, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
            PSHADER(SDF_Pattern_MirrorU, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
            PSHADER(SDF_Pattern_MirrorV, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
            PSHADER(SDF_Pattern_Mirror, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)

          #ifndef NS_PLATFORM_GAME_CORE
            PSHADER(SDF_LCD_Solid, VS_CB0 | VS_CB1 | PS_T3)
            PSHADER(SDF_LCD_Linear, VS_CB0 | VS_CB1 | PS_CB0 | PS_T1 | PS_T3)
            PSHADER(SDF_LCD_Radial, VS_CB0 | VS_CB1 | PS_CB0 | PS_T1 | PS_T3)
            PSHADER(SDF_LCD_Pattern, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
            PSHADER(SDF_LCD_Pattern_Clamp, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
            PSHADER(SDF_LCD_Pattern_Repeat, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
            PSHADER(SDF_LCD_Pattern_MirrorU, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
            PSHADER(SDF_LCD_Pattern_MirrorV, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
            PSHADER(SDF_LCD_Pattern_Mirror, VS_CB0 | VS_CB1 | PS_CB0 | PS_T0 | PS_T3)
          #endif

            PSHADER(Opacity_Solid, VS_CB0 | PS_T2)
            PSHADER(Opacity_Linear, VS_CB0 | PS_CB0 | PS_T1 | PS_T2)
            PSHADER(Opacity_Radial, VS_CB0 | PS_CB0 | PS_T1 | PS_T2)
            PSHADER(Opacity_Pattern, VS_CB0 | PS_CB0 | PS_T0 | PS_T2)
            PSHADER(Opacity_Pattern_Clamp, VS_CB0 | PS_CB0 | PS_T0 | PS_T2)
            PSHADER(Opacity_Pattern_Repeat, VS_CB0 | PS_CB0 | PS_T0 | PS_T2)
            PSHADER(Opacity_Pattern_MirrorU, VS_CB0 | PS_CB0 | PS_T0 | PS_T2)
            PSHADER(Opacity_Pattern_MirrorV, VS_CB0 | PS_CB0 | PS_T0 | PS_T2)
            PSHADER(Opacity_Pattern_Mirror, VS_CB0 | PS_CB0 | PS_T0 | PS_T2)

            PSHADER(Upsample, VS_CB0 | PS_T0 | PS_T2)
            PSHADER(Downsample, VS_CB0 | PS_T0)

            PSHADER(Shadow, VS_CB0 | PS_CB1 | PS_T2 | PS_T4)
            PSHADER(Blur, VS_CB0 | PS_CB1 | PS_T2 | PS_T4)

            default: return ShaderPS{};
        }
    };

    for (uint32_t i = 0; i < Shader::Count; i++)
    {
        const ShaderPS& pShader = pShaders(i);

        if (pShader.label != nullptr)
        {
            mPrograms[i].signature = pShader.signature;
            mPrograms[i].rootSignature = CreateRootSignature(pShader.signature);

            uint8_t vsIndex = VertexForShader[i];

          #ifdef USE_DXC_SHADERS
            const ShaderVS& vs = ShadersVS(VertexForShader_x[i], mCaps.linearRendering, false);
          #else
            const ShaderVS& vs = ShadersVS(vsIndex, mCaps.linearRendering, false);
          #endif

            uint32_t format = FormatForVertex[vsIndex];
            Vector<D3D12_INPUT_ELEMENT_DESC, Shader::Vertex::Format::Attr::Count> elements;
            FillElements(elements, format);
            NS_ASSERT(elements.IsSmall());

            mPrograms[i].stride = SizeForFormat[format];

            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
            desc.pRootSignature = mPrograms[i].rootSignature;
            desc.VS.pShaderBytecode = shaders + vs.start;
            desc.VS.BytecodeLength = vs.size;
            desc.PS.pShaderBytecode = shaders + pShader.start;
            desc.PS.BytecodeLength = pShader.size;
            desc.InputLayout.pInputElementDescs = elements.Data();
            desc.InputLayout.NumElements = elements.Size();
            desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            desc.NumRenderTargets = 1;
            desc.RTVFormats[0] = mColorFormat;
            desc.DSVFormat = mStencilFormat;
            desc.SampleDesc = mSampleDesc;
            desc.SampleMask = 0xFFFFFFFF;

            CreatePipelines(&mPrograms[i], (uint8_t)i, pShader.label, desc, false);

          #if STEREO_RENDERING
            const ShaderVS& vsVR = ShadersVS(vsIndex, mCaps.linearRendering, true);
            desc.VS.pShaderBytecode = shaders + vsVR.start;
            desc.VS.BytecodeLength = vsVR.size;

            mProgramsVR[i].signature = mPrograms[i].signature;
            mProgramsVR[i].rootSignature = mPrograms[i].rootSignature;
            mProgramsVR[i].stride = mPrograms[i].stride;

            CreatePipelines(&mProgramsVR[i], (uint8_t)i, pShader.label, desc, true);
          #endif
        }
        else
        {
            memset(&mPrograms[i], 0, sizeof(mPrograms[i]));

          #if STEREO_RENDERING
            memset(&mProgramsVR[i], 0, sizeof(mPrograms[i]));
          #endif
        }
    }

    Dealloc(shaders);

#ifdef NS_PROFILE
    uint64_t t1 = HighResTimer::Ticks();
    NS_LOG_TRACE("Shaders compiled in %.0f ms", 1000.0 * HighResTimer::Seconds(t1 - t0));
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::DestroyShaders()
{
    for (uint32_t i = 0; i < NS_COUNTOF(mPrograms); i++)
    {
        for (uint32_t j = 0; j < NS_COUNTOF(Program::pso); j++)
        {
            DX_DESTROY(mPrograms[i].pso[j]);
          #if STEREO_RENDERING
            DX_DESTROY(mProgramsVR[i].pso[j]);
          #endif
        }
    }

    for (const Program& p : mCustomShaders)
    {
        for (uint32_t j = 0; j < NS_COUNTOF(Program::pso); j++)
        {
            DX_DESTROY(p.pso[j]);
        }
    }

  #if STEREO_RENDERING
    NS_ASSERT(mCustomShaders.Size() == mCustomShadersVR.Size());
    for (const Program& p : mCustomShadersVR)
    {
        for (uint32_t j = 0; j < NS_COUNTOF(Program::pso); j++)
        {
            DX_DESTROY(p.pso[j]);
        }
    }
  #endif

    for (RootSignatures::Bucket it : mRootSignatures)
    {
        DX_DESTROY(it.value);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderTarget> D3D12RenderDevice::CreateRenderTarget(const char* label, uint32_t width,
    uint32_t height, ID3D12Resource* colorAA, ID3D12Resource* stencil)
{
    NS_LOG_TRACE("RenderTarget '%s' %d x %d %dx", label, width, height, mSampleDesc.Count);

    D3D12_HEAP_PROPERTIES heap;
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap.CreationNodeMask = 0;
    heap.VisibleNodeMask = 0;

    // Texture
    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = mColorFormat;
    desc.SampleDesc = { 1, 0 };
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = colorAA ? D3D12_RESOURCE_FLAG_NONE : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    ID3D12Resource* color;
    V(mDevice->CreateCommittedResource(&heap, mHeapFlags, &desc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, DX_ARGS(&color)));
    DX_NAME(color, "Noesis_%s", label);

    // SRV
    NS_ASSERT(mSRVPool.nextDescriptor < MAX_SRV_DESCRIPTORS);
    uint32_t srv = AllocateDescriptor(mSRVPool);
    mDevice->CreateShaderResourceView(color, nullptr, { mSRVHeapStartCPU + srv * mSRVSize });

    // RTV
    NS_ASSERT(mRTVPool.nextDescriptor < MAX_RTV_DESCRIPTORS);
    uint32_t rtv = AllocateDescriptor(mRTVPool);
    mDevice->CreateRenderTargetView(colorAA ? colorAA : color, 0, { mRTVHeapStart + rtv * mRTVSize });

    // Stencil
    uint32_t dsv = 0xFFFF;

    if (stencil)
    {
        NS_ASSERT(mDSVPool.nextDescriptor < MAX_DSV_DESCRIPTORS);
        dsv = AllocateDescriptor(mDSVPool);
        mDevice->CreateDepthStencilView(stencil, nullptr, { mDSVHeapStart + dsv * mDSVSize });
    }
 
    Ptr<D3D12Texture> texture = *new D3D12Texture();
    texture->device = this;
    texture->obj = color;
    texture->srv = srv;
    texture->width = width;
    texture->height = height;
    texture->levels = 1;
    texture->hasAlpha = true;
    texture->state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    Ptr<D3D12RenderTarget> surface = *new D3D12RenderTarget();
    surface->width = width;
    surface->height = height;
    surface->rtv = rtv;
    surface->dsv = dsv;
    surface->texture = texture;
    surface->colorAA = colorAA;
    surface->stencil = stencil;

    return surface;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::FillCaps(bool sRGB)
{
    mCaps.centerPixelOffset = 0.0f;
    mCaps.linearRendering = sRGB;

#ifndef NS_PLATFORM_GAME_CORE
    mCaps.subpixelRendering = true;
#endif

    mHeapFlags = D3D12_HEAP_FLAG_NONE;

#ifndef NS_PLATFORM_GAME_CORE
    ID3D12Device8* device8;
    if (SUCCEEDED(mDevice->QueryInterface(IID_PPV_ARGS(&device8))))
    {
        // https://devblogs.microsoft.com/directx/coming-to-directx-12-more-control-over-memory-allocation/
        mHeapFlags = D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;
        DX_RELEASE(device8);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::InvalidateStateCache()
{
    mCachedRootSignature = nullptr;
    mCachedPipelineState = nullptr;
    mCachedIndexBuffer = 0;
    mCachedStencilRef = 0xFFFFFFFF;

    ID3D12DescriptorHeap* heaps[] = { mSRVHeap, mSamplerHeap };
    mCommands->SetDescriptorHeaps(2, heaps);

    mCommands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::RebindRootSignatureDescriptors()
{
    if (mCachedRootSignature)
    {
        ID3D12RootSignature* rootSignature = mCachedRootSignature;
        mCachedRootSignature = nullptr;
        SetRootSignature(rootSignature);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::SetRootSignature(ID3D12RootSignature* rootSignature)
{
    NS_ASSERT(rootSignature != nullptr);

    if (mCachedRootSignature != rootSignature)
    {
        mCommands->SetGraphicsRootSignature(rootSignature);
        memset(mCachedRootSignatureParameters, 0, sizeof(mCachedRootSignatureParameters));
        mCachedRootSignature = rootSignature;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::SetPipelineState(ID3D12PipelineState* state)
{
    NS_ASSERT(state != nullptr);

    if (mCachedPipelineState != state)
    {
        mCommands->SetPipelineState(state);
        mCachedPipelineState = state;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::SetIndexBuffer(D3D12_GPU_VIRTUAL_ADDRESS address)
{
    if (mCachedIndexBuffer != address)
    {
        D3D12_INDEX_BUFFER_VIEW view;
        view.BufferLocation = address;
        view.SizeInBytes = mIndices.size;
        view.Format = DXGI_FORMAT_R16_UINT;
        mCommands->IASetIndexBuffer(&view);

        mCachedIndexBuffer = address;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::SetStencilRef(uint32_t stencilRef)
{
    if (mCachedStencilRef != stencilRef)
    {
        mCommands->OMSetStencilRef(stencilRef);
        mCachedStencilRef = stencilRef;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::UploadTexture(uint32_t& index, Texture* texture_, uint32_t sampler)
{
    NS_ASSERT(texture_ != 0);
    D3D12Texture* texture = (D3D12Texture*)texture_;

    EnsureShaderResourceState(mCommands, texture);

    // SRV
    {
        if (NS_UNLIKELY(texture->device == 0))
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC desc;
            desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Texture2D.MipLevels = (UINT)-1;
            desc.Texture2D.MostDetailedMip = 0;
            desc.Texture2D.PlaneSlice = 0;
            desc.Texture2D.ResourceMinLODClamp = 0;

            DXGI_FORMAT format = texture->obj->GetDesc().Format;

            switch (format)
            {
                case DXGI_FORMAT_R32G32B32A32_TYPELESS:
                {
                    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                    break;
                }
                case DXGI_FORMAT_R16G16B16A16_TYPELESS:
                {
                    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                    break;
                }
                case DXGI_FORMAT_R10G10B10A2_TYPELESS:
                {
                    desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
                    break;
                }
                case DXGI_FORMAT_R8G8B8A8_TYPELESS:
                {
                    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    break;
                }
                case DXGI_FORMAT_B8G8R8X8_TYPELESS:
                {
                    desc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
                    break;
                }
                case DXGI_FORMAT_B8G8R8A8_TYPELESS:
                {
                    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                    break;
                }
                case DXGI_FORMAT_BC1_TYPELESS:
                {
                    desc.Format = DXGI_FORMAT_BC1_UNORM;
                    break;
                }
                case DXGI_FORMAT_BC2_TYPELESS:
                {
                    desc.Format = DXGI_FORMAT_BC2_UNORM;
                    break;
                }
                case DXGI_FORMAT_BC3_TYPELESS:
                {
                    desc.Format = DXGI_FORMAT_BC3_UNORM;
                    break;
                }
                case DXGI_FORMAT_BC4_TYPELESS:
                {
                    desc.Format = DXGI_FORMAT_BC4_UNORM;
                    break;
                }
                case DXGI_FORMAT_BC5_TYPELESS:
                {
                    desc.Format = DXGI_FORMAT_BC5_UNORM;
                    break;
                }
                case DXGI_FORMAT_BC7_TYPELESS:
                {
                    desc.Format = DXGI_FORMAT_BC7_UNORM;
                    break;
                }
                default:
                {
                    desc.Format = format;
                    break;
                }
            }

            // This is the first use of a wrapped texture
            NS_ASSERT(mSRVPool.nextDescriptor < MAX_SRV_DESCRIPTORS);
            uint32_t srv = AllocateDescriptor(mSRVPool);
            D3D12_CPU_DESCRIPTOR_HANDLE descriptor = { mSRVHeapStartCPU + srv * mSRVSize };
            mDevice->CreateShaderResourceView(texture->obj, &desc, descriptor);

            texture->LinkDevice(this, srv);
        }

        UINT64 addr = mSRVHeapStartGPU + mSRVSize * texture->srv;

        if (mCachedRootSignatureParameters[index] != addr)
        {
            mCommands->SetGraphicsRootDescriptorTable(index, { addr });
            mCachedRootSignatureParameters[index] = addr;
        }
    }

    // Sampler
    {
        UINT64 addr = mSamplerHeapStartGPU + mSamplerSize * sampler;

        if (mCachedRootSignatureParameters[index + 1] != addr)
        {
            mCommands->SetGraphicsRootDescriptorTable(index + 1, { addr });
            mCachedRootSignatureParameters[index + 1] = addr;
        }
    }

    index += 2;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::UploadUniforms(uint32_t& index, uint32_t i, const UniformData* data)
{
    NS_ASSERT(data != 0);
    NS_ASSERT(data->numDwords > 0);

    if (mCachedConstantHash[i] != data->hash)
    {
        uint32_t size = data->numDwords * sizeof(uint32_t);
        uint32_t alignedSize = Align(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        void* ptr = MapBuffer(mConstants[i], alignedSize);
        memcpy(ptr, data->values, size);

        mCachedConstantAddress[i] = mConstants[i].currentPage->gpuAddress + mConstants[i].drawPos;
        mCachedConstantHash[i] = data->hash;
    }

    if (mCachedRootSignatureParameters[index] != mCachedConstantAddress[i])
    {
        mCommands->SetGraphicsRootConstantBufferView(index, mCachedConstantAddress[i]);
        mCachedRootSignatureParameters[index] = mCachedConstantAddress[i];
    }

    index++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::SetDescriptors(const Batch& batch, uint32_t signature)
{
    uint32_t index = 0;

    if (signature & VS_CB0) UploadUniforms(index, 0, &batch.vertexUniforms[0]);
    if (signature & VS_CB1) UploadUniforms(index, 1, &batch.vertexUniforms[1]);
    if (signature & PS_CB0) UploadUniforms(index, 2, &batch.pixelUniforms[0]);
    if (signature & PS_CB1) UploadUniforms(index, 3, &batch.pixelUniforms[1]);

    if (signature & PS_T0) UploadTexture(index, batch.pattern, batch.patternSampler.v);
    if (signature & PS_T1) UploadTexture(index, batch.ramps, batch.rampsSampler.v);
    if (signature & PS_T2) UploadTexture(index, batch.image, batch.imageSampler.v);
    if (signature & PS_T3) UploadTexture(index, batch.glyphs, batch.glyphsSampler.v);
    if (signature & PS_T4) UploadTexture(index, batch.shadow, batch.shadowSampler.v);

    NS_ASSERT(index < NS_COUNTOF(mCachedRootSignatureParameters));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderDevice::SetBuffers(const Batch& batch, uint32_t stride)
{
    SetIndexBuffer(mIndices.currentPage->gpuAddress);

    UINT64 offset = (UINT64)mVertices.drawPos + batch.vertexOffset;

    D3D12_VERTEX_BUFFER_VIEW view;
    view.BufferLocation = mVertices.currentPage->gpuAddress + offset;
    view.SizeInBytes = stride * batch.numVertices;
    view.StrideInBytes = stride;
    mCommands->IASetVertexBuffers(0, 1, &view);
}
