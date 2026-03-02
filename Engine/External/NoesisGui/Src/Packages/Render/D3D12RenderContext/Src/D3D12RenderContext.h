////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __RENDER_D3D12RENDERCONTEXT_H__
#define __RENDER_D3D12RENDERCONTEXT_H__


#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/Delegate.h>
#include <NsRender/RenderContext.h>

#ifdef NS_PLATFORM_GAME_CORE
  #ifndef NS_PROFILE
    #define D3DCOMPILE_NO_DEBUG 1
  #endif
  #ifdef NS_PLATFORM_SCARLETT
    #include <d3d12_xs.h>
  #endif
  #ifdef NS_PLATFORM_XBOX_ONE_CORE
    #include <d3d12_x.h>
  #endif
#else
  #include "directx/d3d12.h"
  #include "directx/dxgi1_5.h"
#endif


namespace Noesis { class RenderDevice; }

namespace NoesisApp
{

////////////////////////////////////////////////////////////////////////////////////////////////////
class D3D12RenderContext final: public RenderContext
{
public:
    D3D12RenderContext();
    ~D3D12RenderContext();

    /// From RenderContext
    //@{
    const char* Description() const override;
    const char* ShaderLang() const override;
    uint32_t Score() const override;
    bool Validate() const override;
    void Init(void* window, uint32_t& samples, bool vsync, bool sRGB) override;
    void Shutdown() override;
    Noesis::RenderDevice* GetDevice() const override;
    void BeginRender() override;
    void EndRender() override;
    void Resize() override;
    float GetGPUTimeMs() const override;
    void SetClearColor(float r, float g, float b, float a) override;
    void SetDefaultRenderTarget(uint32_t width, uint32_t height, bool doClearColor) override;
    Noesis::Ptr<Image> CaptureRenderTarget(Noesis::RenderTarget* surface) const override;
    void Swap() override;
    void* CreatePixelShader(const char* label, uint8_t shader,
        Noesis::ArrayRef<uint8_t> code) override;
    //@}

private:
    void CreateDevice();
    void CreateSwapChain(void* window, bool vsync);
    void CreateCommandQueue();
    void CreateHeaps();
    void CreateQueries();
    void CreateBuffers(bool createStencil);
    void CreateCommandList();

    DXGI_SAMPLE_DESC GetSampleDesc(uint32_t samples) const;

    void WaitForGpu();

    friend class D3D12MediaPlayer;
    void InsertWait(ID3D12CommandQueue* queue);

    Noesis::Delegate<void (ID3D12GraphicsCommandList*)> mTransitionVideoTextures;

private:
    Noesis::Ptr<Noesis::RenderDevice> mRenderer;

  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    HMODULE mD3D12;
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = nullptr;
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = nullptr;

    HMODULE mDXGI;
    typedef HRESULT (WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT, REFIID, _COM_Outptr_ void**);
    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2 = nullptr;
  #endif

  #ifndef NS_PLATFORM_GAME_CORE
    UINT mSwapChainFlags = 0;
    UINT mPresentFlags = 0;
    UINT mSyncInterval = 0;
    IDXGISwapChain3* mSwapChain = nullptr;
  #endif

  #ifdef NS_PLATFORM_GAME_CORE
    D3D12XBOX_PRESENT_PARAMETERS mPresentParameters = {};
    D3D12XBOX_FRAME_PIPELINE_TOKEN mFramePipelineToken;
  #endif

    // The maximum number of frames that will be queued to the GPU at a time, as well as the number
    // of back buffers in the DXGI swap chain
    static const uint32_t FrameCount = 2;

    ID3D12Device* mDevice = nullptr;
    ID3D12CommandQueue* mQueue = nullptr;
    ID3D12DescriptorHeap* mRTVHeap = nullptr;
    ID3D12DescriptorHeap* mDSVHeap = nullptr;
    ID3D12QueryHeap* mQueryHeap = nullptr;
    ID3D12Resource* mTimeStampResults = nullptr;
    ID3D12Resource* mStencilBuffer = nullptr;
    ID3D12Resource* mBackBuffers[FrameCount] = {};
    ID3D12Resource* mBackBuffersAA[FrameCount] = {};
    ID3D12CommandAllocator* mCommandAllocators[FrameCount] = {};
    ID3D12GraphicsCommandList* mCommands;
    ID3D12Fence* mFence = nullptr;
    HANDLE mFenceEvent = 0;
    UINT mFrameIndex = 0;

    UINT mRTVSize = 0;

    uint64_t mFenceValue = 0;
    uint64_t mFrameFenceValues[FrameCount] = {};

    DXGI_FORMAT mFormat;
    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissorRect;
    UINT32 mWidth, mHeight;
    FLOAT mClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    DXGI_SAMPLE_DESC mSampleDesc;

    float mGPUTime = 0.0f;
    UINT64 mTimeStampFrequency;

    NS_DECLARE_REFLECTION(D3D12RenderContext, RenderContext)
};

}

#endif
