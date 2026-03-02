////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include "D3D12RenderContext.h"

#include <NsCore/ReflectionImplement.h>
#include <NsCore/Category.h>
#include <NsCore/Log.h>
#include <NsCore/UTF8.h>
#include <NsDrawing/Color.h>
#include <NsRender/D3D12Factory.h>
#include <NsRender/RenderDevice.h>
#include <NsRender/Image.h>
#include <NsRender/Texture.h>
#include <NsRender/RenderTarget.h>

#ifdef NS_PLATFORM_GAME_CORE
    #include <pix3.h>
#else
    #include "directx/pix.h"
#endif

#ifdef NS_PLATFORM_WINDOWS_DESKTOP
#include "D3D12MediaPlayer.h"
#include <NsApp/MediaElement.h>
#endif


using namespace Noesis;
using namespace NoesisApp;


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

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D12RenderContext::D3D12RenderContext()
{
#ifdef NS_PLATFORM_WINDOWS_DESKTOP
    mD3D12 = LoadLibraryA("d3d12.dll");
    if (mD3D12 != 0)
    {
        D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(mD3D12, "D3D12CreateDevice");
        D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(mD3D12, "D3D12GetDebugInterface");
    }

    mDXGI = LoadLibraryA("dxgi.dll");
    if (mDXGI != 0)
    {
        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(mDXGI, "CreateDXGIFactory2");
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D12RenderContext::~D3D12RenderContext()
{
#ifdef NS_PLATFORM_WINDOWS_DESKTOP
    if (mD3D12 != 0)
    {
        FreeLibrary(mD3D12);
    }

    if (mDXGI != 0)
    {
        FreeLibrary(mDXGI);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* D3D12RenderContext::Description() const
{
#if defined(NS_PLATFORM_SCARLETT)
    return "Xbox Series";
#elif defined(NS_PLATFORM_XBOX_ONE_CORE)
    return "Xbox One";
#else
    return "D3D12";
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* D3D12RenderContext::ShaderLang() const
{
    return "hlsl";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t D3D12RenderContext::Score() const
{
    return 300;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3D12RenderContext::Validate() const
{
#ifndef NS_PLATFORM_GAME_CORE
    if (D3D12CreateDevice && CreateDXGIFactory2)
    {
        return D3D12CreateDevice(0, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), 0) == S_FALSE;
    }

    return false;
#else
    return true;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::Init(void* window, uint32_t& samples, bool vsync, bool sRGB)
{
    NS_ASSERT(Validate());
    NS_LOG_DEBUG("Creating D3D12 render context");

    CreateDevice();
    CreateCommandQueue();
    CreateSwapChain(window, vsync);

    mFormat = sRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    mSampleDesc = GetSampleDesc(samples);
    samples = mSampleDesc.Count;

    CreateHeaps();
    CreateQueries();
    CreateCommandList();

    mRenderer = D3D12Factory::CreateDevice(mDevice, mFence, mFormat, DXGI_FORMAT_D24_UNORM_S8_UINT, 
        mSampleDesc, sRGB);

#ifdef NS_PLATFORM_WINDOWS_DESKTOP
    D3D12MediaPlayer::Init();
    MediaElement::SetCreateMediaPlayerCallback(&D3D12MediaPlayer::Create, this);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::Shutdown()
{
#ifdef NS_PLATFORM_WINDOWS_DESKTOP
    D3D12MediaPlayer::Shutdown();
#endif

    WaitForGpu();
    mRenderer.Reset();

    BOOL r = CloseHandle(mFenceEvent);
    NS_ASSERT(r != 0);

    for (uint32_t i = 0; i < FrameCount; i++)
    {
        DX_RELEASE(mBackBuffers[i]);
        DX_RELEASE(mBackBuffersAA[i]);
    }

#ifndef NS_PLATFORM_GAME_CORE
    DX_RELEASE(mSwapChain);
#endif

    DX_RELEASE(mStencilBuffer);
    DX_RELEASE(mQueue);

    DX_DESTROY(mFence);
    DX_DESTROY(mCommands);
    DX_DESTROY(mRTVHeap);
    DX_DESTROY(mDSVHeap);
    DX_DESTROY(mQueryHeap);
    DX_DESTROY(mTimeStampResults);

    for (uint32_t i = 0; i < FrameCount; i++)
    {
        DX_DESTROY(mCommandAllocators[i]);
    }

    DX_DESTROY(mDevice);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
RenderDevice* D3D12RenderContext::GetDevice() const
{
    return mRenderer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::BeginRender()
{
  #ifdef NS_PLATFORM_GAME_CORE
    mFramePipelineToken = D3D12XBOX_FRAME_PIPELINE_TOKEN_NULL;
    V(mDevice->WaitFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, INFINITE, nullptr,
      D3D12XBOX_WAIT_FRAME_EVENT_FLAG_NONE, &mFramePipelineToken));
  #endif

    // Wait for the command buffer to be fully processed
    if (mFence->GetCompletedValue() < mFrameFenceValues[mFrameIndex])
    {
        V(mFence->SetEventOnCompletion(mFrameFenceValues[mFrameIndex], mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
    }

#ifdef NS_PROFILE
    D3D12_RANGE emptyRange{};
    D3D12_RANGE range;
    range.Begin = sizeof(UINT64) * 2 * mFrameIndex;
    range.End = range.Begin + 2 * sizeof(UINT64);

    void* data;
    V(mTimeStampResults->Map(0, &range, &data));
    const UINT64* timeStamps = (UINT64*)((uint8_t*)data + range.Begin);
    const UINT64 delta = timeStamps[1] - timeStamps[0];
    mGPUTime = 1000.0f * delta / mTimeStampFrequency;
    mTimeStampResults->Unmap(0, &emptyRange);
#endif

    V(mCommandAllocators[mFrameIndex]->Reset());
    V(mCommands->Reset(mCommandAllocators[mFrameIndex], nullptr));

#ifdef NS_PROFILE
    mCommands->EndQuery(mQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 2 * mFrameIndex);
#endif

    D3D12Factory::SetCommandList(mRenderer, mCommands, mFenceValue + 1);

    DX_BEGIN_EVENT("Frame #%d", gFlipCount++);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::EndRender()
{
    D3D12Factory::EndPendingSplitBarriers(mRenderer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::Resize()
{
    WaitForGpu();

    for (uint32_t i = 0; i < FrameCount; i++)
    {
        DX_RELEASE(mBackBuffers[i]);
        DX_RELEASE(mBackBuffersAA[i]);
    }

    DX_RELEASE(mStencilBuffer);

  #ifndef NS_PLATFORM_GAME_CORE
    V(mSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, mSwapChainFlags));
  #endif

    CreateBuffers(true);

    mViewport.TopLeftX = 0.0f;
    mViewport.TopLeftY = 0.0f;
    mViewport.Width = (FLOAT)mWidth;
    mViewport.Height = (FLOAT)mHeight;
    mViewport.MinDepth = 0.0f;
    mViewport.MaxDepth = 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float D3D12RenderContext::GetGPUTimeMs() const
{
    return mGPUTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::SetClearColor(float r, float g, float b, float a)
{
    bool linear = mFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    mClearColor[0] = linear ? SRGBToLinear(r) : r;
    mClearColor[1] = linear ? SRGBToLinear(g) : g;
    mClearColor[2] = linear ? SRGBToLinear(b) : b;
    mClearColor[3] = a;

    // Surfaces need to be recreated to update Optimized Clear Value
    if (mBackBuffers[0] != nullptr)
    {
      #ifndef NS_PLATFORM_GAME_CORE
        if (mSampleDesc.Count > 1)
      #endif
        {
            WaitForGpu();

            for (uint32_t i = 0; i < FrameCount; i++)
            {
                DX_RELEASE(mBackBuffers[i]);
                DX_RELEASE(mBackBuffersAA[i]);
            }

            // No need to recreate stencil buffer (Optimized Clear Value is always zero)
            CreateBuffers(false);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::SetDefaultRenderTarget(uint32_t, uint32_t, bool doClearColor)
{
    DX_BEGIN_EVENT("MainRenderTarget");

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.Subresource = 0;

    if (mSampleDesc.Count > 1)
    {
        barrier.Transition.pResource = mBackBuffersAA[mFrameIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    else
    {
        barrier.Transition.pResource = mBackBuffers[mFrameIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }

    mCommands->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = mRTVHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += mFrameIndex * mRTVSize;

    if (doClearColor)
    {
        mCommands->ClearRenderTargetView(rtv, mClearColor, 0, nullptr);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsv = mDSVHeap->GetCPUDescriptorHandleForHeapStart();
    mCommands->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);

    mCommands->OMSetRenderTargets(1, &rtv, false, &dsv);
    mCommands->RSSetViewports(1, &mViewport);
    mCommands->RSSetScissorRects(1, &mScissorRect);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<NoesisApp::Image> D3D12RenderContext::CaptureRenderTarget(RenderTarget* surface_) const
{
    D3D12Factory::EndPendingSplitBarriers(mRenderer);

    ID3D12Resource* texture = D3D12Factory::GetNativePtr(surface_->GetTexture());
    D3D12_RESOURCE_DESC textureDesc = texture->GetDesc();

    // Transition from PIXEL_SHADER_RESOURCE to COPY_SOURCE
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.Subresource = 0;
    barrier.Transition.pResource = texture;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

    mCommands->ResourceBarrier(1, &barrier);

    UINT64 rowSize = 0;
    mDevice->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, &rowSize, nullptr);
    UINT64 pitch = (rowSize + 255) & ~255;

    // Create staging texture
    D3D12_HEAP_PROPERTIES heap;
    heap.Type = D3D12_HEAP_TYPE_READBACK;
    heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap.CreationNodeMask = 0;
    heap.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = pitch * textureDesc.Height * 4;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc = { 1, 0 };
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource* stagingTexture;
    V(mDevice->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
    D3D12_RESOURCE_STATE_COPY_DEST, nullptr, DX_ARGS(&stagingTexture)));

    // Copy texture -> staging
    D3D12_TEXTURE_COPY_LOCATION copySrc;
    copySrc.pResource = texture;
    copySrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    copySrc.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION copyDest;
    copyDest.pResource = stagingTexture;
    copyDest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    copyDest.PlacedFootprint.Offset = 0;
    copyDest.PlacedFootprint.Footprint.Width = 
    copyDest.PlacedFootprint.Footprint.Width = (UINT)textureDesc.Width;
    copyDest.PlacedFootprint.Footprint.Height = (UINT)textureDesc.Height;
    copyDest.PlacedFootprint.Footprint.Depth = 1;
    copyDest.PlacedFootprint.Footprint.RowPitch = (UINT)pitch;
    copyDest.PlacedFootprint.Footprint.Format = textureDesc.Format;

    // Wait GPU completion
    mCommands->CopyTextureRegion(&copyDest, 0, 0, 0, &copySrc, nullptr);
    V(mCommands->Close());
    mQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&mCommands);
    ((D3D12RenderContext*)this)->WaitForGpu();

    // Map and copy to image
    void* data;
    V(stagingTexture->Map(0, nullptr, &data));

    Ptr<Image> image = *new Image((uint32_t)textureDesc.Width, (uint32_t)textureDesc.Height);
    const uint8_t* src = (const uint8_t*)data;
    uint8_t* dst = (uint8_t*)image->Data();

    for (uint32_t i = 0; i < textureDesc.Height; i++)
    {
        for (uint32_t j = 0; j < textureDesc.Width; j++)
        {
            // RGBA -> BGRA
            dst[4 * j + 0] = src[4 * j + 2];
            dst[4 * j + 1] = src[4 * j + 1];
            dst[4 * j + 2] = src[4 * j + 0];
            dst[4 * j + 3] = src[4 * j + 3];
        }

        src += pitch;
        dst += image->Stride();
    }
   
    D3D12_RANGE range = { };
    stagingTexture->Unmap(0, &range);
    DX_DESTROY(stagingTexture);

  #ifdef NS_PLATFORM_GAME_CORE
    D3D12XBOX_PRESENT_PLANE_PARAMETERS planeParameters{};
    planeParameters.Token = mFramePipelineToken;
    V(mQueue->PresentX(1, &planeParameters, nullptr));
  #endif

    return image;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::Swap()
{
#ifdef NS_PROFILE
    mCommands->EndQuery(mQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 2 * mFrameIndex + 1);
    mCommands->ResolveQueryData(mQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 2 * mFrameIndex, 2,
        mTimeStampResults, mFrameIndex * sizeof(UINT64) * 2);
#endif

    // SetDefaultRenderTarget
    DX_END_EVENT();

    // BeginRender
    DX_END_EVENT();

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.Subresource = 0;

    if (mSampleDesc.Count > 1)
    {
        ID3D12Resource* src = mBackBuffersAA[mFrameIndex];
        ID3D12Resource* dst = mBackBuffers[mFrameIndex];

        barrier.Transition.pResource = src;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        mCommands->ResourceBarrier(1, &barrier);

        barrier.Transition.pResource = dst;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_DEST;
        mCommands->ResourceBarrier(1, &barrier);

        mCommands->ResolveSubresource(dst, 0, src, 0, mFormat);

        barrier.Transition.pResource = dst;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        mCommands->ResourceBarrier(1, &barrier);
    }
    else
    {
        barrier.Transition.pResource = mBackBuffers[mFrameIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        mCommands->ResourceBarrier(1, &barrier);
    }

    mTransitionVideoTextures(mCommands);

    V(mCommands->Close());
    mQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&mCommands);

    mFenceValue++;
    V(mQueue->Signal(mFence, mFenceValue));
    mFrameFenceValues[mFrameIndex] = mFenceValue;

#ifdef NS_PLATFORM_GAME_CORE
    D3D12XBOX_PRESENT_PLANE_PARAMETERS planeParameters{};
    planeParameters.Token = mFramePipelineToken;
    planeParameters.ResourceCount = 1;
    planeParameters.ppResources = &mBackBuffers[mFrameIndex];
    V(mQueue->PresentX(1, &planeParameters, &mPresentParameters));
    mFrameIndex = (mFrameIndex + 1) % FrameCount;
#else
    V(mSwapChain->Present(mSyncInterval, mPresentFlags));
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* D3D12RenderContext::CreatePixelShader(const char* label, uint8_t shader,
    ArrayRef<uint8_t> code)
{
    return D3D12Factory::CreatePixelShader(mRenderer, label, shader, code.Data(), code.Size());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::CreateDevice()
{
#ifdef NS_PLATFORM_GAME_CORE
    D3D12XBOX_CREATE_DEVICE_PARAMETERS params{};
    params.Version = D3D12_SDK_VERSION;

   #ifdef NS_DEBUG
    // Enable the debug layer
    params.ProcessDebugFlags |= D3D12_PROCESS_DEBUG_FLAG_DEBUG_LAYER_ENABLED;
   #endif

   #ifdef NS_PROFILE
    // Enable the instrumented driver
    params.ProcessDebugFlags |= D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED;
   #endif

    params.GraphicsCommandQueueRingSizeBytes = D3D12XBOX_DEFAULT_SIZE_BYTES;
    params.GraphicsScratchMemorySizeBytes = D3D12XBOX_DEFAULT_SIZE_BYTES;
    params.ComputeScratchMemorySizeBytes = D3D12XBOX_DEFAULT_SIZE_BYTES;

    V(D3D12XboxCreateDevice(nullptr, &params, DX_ARGS(&mDevice)));

#else
  #ifdef NS_DEBUG
    if (D3D12GetDebugInterface)
    {
        ID3D12Debug* debug;
        if (SUCCEEDED(D3D12GetDebugInterface(DX_ARGS(&debug))))
        {
            debug->EnableDebugLayer();

        #if 0
            ID3D12Debug1* debug1 = 0;
            V(debug->QueryInterface(DX_ARGS(&debug1)));
            debug1->SetEnableGPUBasedValidation(true);
            DX_RELEASE(debug1);
        #endif

            DX_RELEASE(debug);
        }
    }
  #endif

    V(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, DX_ARGS(&mDevice)));
    DX_NAME(mDevice, "Noesis_Device");

    NS_LOG_DEBUG(" Feature Level: %s", [&]()
    {
        const D3D_FEATURE_LEVEL levels_[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        D3D12_FEATURE_DATA_FEATURE_LEVELS levels =
        {
            _countof(levels_), levels_, D3D_FEATURE_LEVEL_11_0
        };

        D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
        HRESULT hr = mDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &levels, sizeof(levels));
        if (SUCCEEDED(hr))
        {
            level = levels.MaxSupportedFeatureLevel;
        }

        static char description[128];
        snprintf(description, 128, "%d_%d", (level >> 12) & 0xf, (level >> 8) & 0xf);
        return description;
    }());
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::CreateSwapChain(void* window, bool vsync)
{
#ifdef NS_PLATFORM_GAME_CORE
    RECT rect;
    BOOL ret = GetClientRect((HWND)window, &rect);
    NS_ASSERT(ret != 0);

    mWidth = rect.right - rect.left;
    mHeight = rect.bottom - rect.top;

    V(mDevice->SetFrameIntervalX(nullptr, D3D12XBOX_FRAME_INTERVAL_60_HZ, FrameCount - 1,
        D3D12XBOX_FRAME_INTERVAL_FLAG_NONE));

    V(mDevice->ScheduleFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, 0U, nullptr,
        D3D12XBOX_SCHEDULE_FRAME_EVENT_FLAG_NONE));

    if (!vsync)
    {
        mPresentParameters.Flags = D3D12XBOX_PRESENT_FLAG_IMMEDIATE;
    }
#else
    UINT flags = 0;

  #ifdef NS_DEBUG
    flags |= DXGI_CREATE_FACTORY_DEBUG;
  #endif

    IDXGIFactory4* factory;
    V(CreateDXGIFactory2(flags, DX_ARGS(&factory)));

    NS_LOG_DEBUG(" Adapter: %s", [&]()
    {
        IDXGIAdapter1* adapter;
        V(factory->EnumAdapterByLuid(mDevice->GetAdapterLuid(), DX_ARGS(&adapter)));

        DXGI_ADAPTER_DESC desc;
        V(adapter->GetDesc(&desc));

        static char description[128];
        uint32_t numChars = UTF8::UTF16To8((uint16_t*)desc.Description, description, 128);
        NS_ASSERT(numChars <= 128);

        DX_RELEASE(adapter);

        return description;
    }());

    if (!vsync)
    {
        IDXGIFactory5* factory_;

        if (SUCCEEDED(factory->QueryInterface(DX_ARGS(&factory_))))
        {
            BOOL tearing = FALSE;
            HRESULT hr = factory_->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(BOOL));
            DX_RELEASE(factory_);

            if (SUCCEEDED(hr) && tearing)
            {
                mSwapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                mPresentFlags = DXGI_PRESENT_ALLOW_TEARING;
            }
        }
    }

    mSyncInterval = vsync ? 1 : 0;

    // SwapChain creation
    DXGI_SWAP_CHAIN_DESC1 desc;
    desc.Width = 0;
    desc.Height = 0;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Stereo = false;
    desc.SampleDesc = { 1, 0 };
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = FrameCount;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags = mSwapChainFlags;

    IDXGISwapChain1* swapChain;
    V(factory->CreateSwapChainForHwnd(mQueue, (HWND)window, &desc, 0, 0, &swapChain));
    V(factory->MakeWindowAssociation((HWND)window, DXGI_MWA_NO_ALT_ENTER));
    V(swapChain->QueryInterface(DX_ARGS(&mSwapChain)));
    DX_RELEASE(swapChain);
    DX_RELEASE(factory);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    V(mDevice->CreateCommandQueue(&queueDesc, DX_ARGS(&mQueue)));
    DX_NAME(mQueue, "Noesis_Queue");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::CreateHeaps()
{
    mRTVSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = FrameCount;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 0;

        V(mDevice->CreateDescriptorHeap(&desc, DX_ARGS(&mRTVHeap)));
        DX_NAME(mRTVHeap, "Noesis_FB_RTV_Heap");
    }
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 0;
        V(mDevice->CreateDescriptorHeap(&desc, DX_ARGS(&mDSVHeap)));
        DX_NAME(mDSVHeap, "Noesis_FB_DSV_Heap");
    }
    {
        D3D12_QUERY_HEAP_DESC desc;
        desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        desc.Count = FrameCount * 2;
        desc.NodeMask = 0;
        V(mDevice->CreateQueryHeap(&desc, DX_ARGS(&mQueryHeap)));
        DX_NAME(mQueryHeap, "Noesis_Query_Heap");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::CreateQueries()
{
    D3D12_HEAP_PROPERTIES heap;
    heap.Type = D3D12_HEAP_TYPE_READBACK;
    heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap.CreationNodeMask = 0;
    heap.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = sizeof(UINT64) * FrameCount * 2;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc = { 1, 0 };
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    V(mDevice->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, DX_ARGS(&mTimeStampResults)));
    DX_NAME(mTimeStampResults, "Noesis_TimeStamp");

    V(mQueue->GetTimestampFrequency(&mTimeStampFrequency));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::CreateBuffers(bool createStencil)
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = mRTVHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;

  #ifndef NS_PLATFORM_GAME_CORE
    NS_ASSERT(mSwapChain != 0);
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    V(mSwapChain->GetDesc1(&swapChainDesc));

    mWidth = swapChainDesc.Width;
    mHeight = swapChainDesc.Height;

    ID3D12Device8* device_;
    if (SUCCEEDED(mDevice->QueryInterface(DX_ARGS(&device_))))
    {
        // https://devblogs.microsoft.com/directx/coming-to-directx-12-more-control-over-memory-allocation/
        flags = D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;
        DX_RELEASE(device_);
    }
  #endif

    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = mWidth;
    desc.Height = mHeight;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    D3D12_HEAP_PROPERTIES heap;
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap.CreationNodeMask = 0;
    heap.VisibleNodeMask = 0;

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = mFormat;
    clearValue.Color[0] = mClearColor[0];
    clearValue.Color[1] = mClearColor[1];
    clearValue.Color[2] = mClearColor[2];
    clearValue.Color[3] = mClearColor[3];

    for (uint32_t i = 0; i < FrameCount; i++)
    {
        desc.Format = mFormat;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

      #ifdef NS_PLATFORM_GAME_CORE
        desc.SampleDesc = { 1, 0 };
        V(mDevice->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_ALLOW_DISPLAY, &desc,
            D3D12_RESOURCE_STATE_PRESENT, &clearValue, DX_ARGS(&mBackBuffers[i])));
      #else
        V(mSwapChain->GetBuffer(i, DX_ARGS(&mBackBuffers[i])));
      #endif

        DX_NAME(mBackBuffers[i], "Noesis_FB_Color_%d", i);

        if (mSampleDesc.Count != 1)
        {
            desc.SampleDesc = mSampleDesc;
            V(mDevice->CreateCommittedResource(&heap, flags, &desc,
                D3D12_RESOURCE_STATE_RESOLVE_SOURCE, &clearValue, DX_ARGS(&mBackBuffersAA[i])));
            DX_NAME(mBackBuffersAA[i], "Noesis_FB_ColorAA_%d", i);

            mDevice->CreateRenderTargetView(mBackBuffersAA[i], nullptr, rtv);
        }
        else
        {
            D3D12_RENDER_TARGET_VIEW_DESC view;
            view.Format = mFormat;
            view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            view.Texture2D.MipSlice = 0;
            view.Texture2D.PlaneSlice = 0;

            mDevice->CreateRenderTargetView(mBackBuffers[i], &view, rtv);
        }

        rtv.ptr += mRTVSize;
    }

    if (createStencil)
    {
        D3D12_CLEAR_VALUE depthClearValue{};
        depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthClearValue.DepthStencil.Depth = 0.0f;
        depthClearValue.DepthStencil.Stencil = 0;

        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        desc.SampleDesc = mSampleDesc;

        V(mDevice->CreateCommittedResource(&heap, flags, &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthClearValue, DX_ARGS(&mStencilBuffer)));
        DX_NAME(mStencilBuffer, "Noesis_FB_Stencil");

        D3D12_CPU_DESCRIPTOR_HANDLE dsv = mDSVHeap->GetCPUDescriptorHandleForHeapStart();
        mDevice->CreateDepthStencilView(mStencilBuffer, nullptr, dsv);
    }

    mScissorRect.left = 0;
    mScissorRect.top = 0;
    mScissorRect.right = (ULONG)desc.Width;
    mScissorRect.bottom = (ULONG)desc.Height;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::CreateCommandList()
{
    // Command allocators
    for (uint32_t i = 0; i < FrameCount; i++)
    {
        V(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
            DX_ARGS(&mCommandAllocators[i])));
        DX_NAME(mCommandAllocators[i], "Noesis_Commands_Allocator_%d", i);
    }

    // Command list
    V(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocators[0], nullptr,
        DX_ARGS(&mCommands)));
    V(mCommands->Close());
    DX_NAME(mCommands, "Noesis_Commands");

    // Fence
    V(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, DX_ARGS(&mFence)));
    DX_NAME(mFence, "Noesis_Fence");

    mFenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    NS_ASSERT(mFenceEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
DXGI_SAMPLE_DESC D3D12RenderContext::GetSampleDesc(uint32_t samples) const
{
    NS_ASSERT(mDevice != 0);

    DXGI_SAMPLE_DESC descs[16];
    samples = Max(1U, Min(samples, 16U));

    descs[0].Count = 1;
    descs[0].Quality = 0;

    for (uint32_t i = 1, last = 0; i < samples; i++)
    {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS levels;
        levels.Format = mFormat;
        levels.SampleCount = i + 1;
        levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        levels.NumQualityLevels = 0;

        V(mDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &levels, sizeof(levels)));

        if (levels.NumQualityLevels > 0)
        {
            descs[i].Count = levels.SampleCount;
            descs[i].Quality = 0;
            last = i;
        }
        else
        {
            descs[i] = descs[last];
        }
    }

    return descs[samples - 1];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::WaitForGpu()
{
    mFenceValue++;
    V(mFence->SetEventOnCompletion(mFenceValue, mFenceEvent));
    V(mQueue->Signal(mFence, mFenceValue));
    WaitForSingleObject(mFenceEvent, INFINITE);

  #ifdef NS_PLATFORM_GAME_CORE
    V(mQueue->PresentX(0, nullptr, nullptr));
  #endif

    memset(mFrameFenceValues, sizeof(mFrameFenceValues), 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12RenderContext::InsertWait(ID3D12CommandQueue* queue)
{
    queue->Wait(mFence, mFenceValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(D3D12RenderContext)
{
    NsMeta<Category>("RenderContext");
}

NS_END_COLD_REGION
