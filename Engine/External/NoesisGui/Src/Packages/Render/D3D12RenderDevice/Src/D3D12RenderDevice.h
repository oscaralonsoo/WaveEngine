////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __RENDER_D3D12RENDERDEVICE_H__
#define __RENDER_D3D12RENDERDEVICE_H__


#include <NsRender/RenderDevice.h>
#include <NsCore/Vector.h>
#include <NsCore/HashMap.h>

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
#endif


namespace Noesis { template<class T> class Ptr; }

namespace NoesisApp
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// D3D12RenderDevice
////////////////////////////////////////////////////////////////////////////////////////////////////
class D3D12RenderDevice final: public Noesis::RenderDevice
{
public:
    D3D12RenderDevice(ID3D12Device* device, ID3D12Fence* frameFence, DXGI_FORMAT colorFormat,
        DXGI_FORMAT stencilFormat, DXGI_SAMPLE_DESC sampleDesc, bool sRGB);
    ~D3D12RenderDevice();

    static Noesis::Ptr<Noesis::Texture> WrapTexture(ID3D12Resource* texture, uint32_t width,
        uint32_t height, uint32_t levels, bool isInverted, bool hasAlpha,
        D3D12_RESOURCE_STATES state);

    void* CreatePixelShader(const char* label, uint8_t shader, const void* hlsl, uint32_t size);
    void ClearPixelShaders();

    void SetCommandList(ID3D12GraphicsCommandList* commands, uint64_t safeFenceValue);
    void SafeReleaseTexture(Noesis::Texture* texture);
    void SafeReleaseRenderTarget(Noesis::RenderTarget* surface);
    void SafeReleaseResource(ID3D12Resource* resource);

    void EndPendingSplitBarriers();

    static ID3D12Resource* GetNativePtr(Noesis::Texture* texture);
    static D3D12_RESOURCE_STATES GetTextureState(Noesis::Texture* texture);
    static void SetTextureState(Noesis::Texture* texture, D3D12_RESOURCE_STATES state);

private:
    /// From RenderDevice
    //@{
    const Noesis::DeviceCaps& GetCaps() const override;
    Noesis::Ptr<Noesis::RenderTarget> CreateRenderTarget(const char* label, uint32_t width,
        uint32_t height, uint32_t sampleCount, bool needsStencil) override;
    Noesis::Ptr<Noesis::RenderTarget> CloneRenderTarget(const char* label,
        Noesis::RenderTarget* surface) override;
    Noesis::Ptr<Noesis::Texture> CreateTexture(const char* label, uint32_t width, uint32_t height,
        uint32_t numLevels, Noesis::TextureFormat::Enum format, const void** data) override;
    void UpdateTexture(Noesis::Texture* texture, uint32_t level, uint32_t x, uint32_t y,
        uint32_t width, uint32_t height, const void* data) override;
    void BeginOffscreenRender() override;
    void EndOffscreenRender() override;
    void BeginOnscreenRender() override;
    void EndOnscreenRender() override;
    void SetRenderTarget(Noesis::RenderTarget* surface) override;
    void BeginTile(Noesis::RenderTarget* surface, const Noesis::Tile& tile) override;
    void EndTile(Noesis::RenderTarget* surface) override;
    void ResolveRenderTarget(Noesis::RenderTarget* surface, const Noesis::Tile* tiles,
        uint32_t numTiles) override;
    void* MapVertices(uint32_t bytes) override;
    void UnmapVertices() override;
    void* MapIndices(uint32_t bytes) override;
    void UnmapIndices() override;
    void DrawBatch(const Noesis::Batch& batch) override;
    //@}

private:
    struct Page;
    struct PageAllocator;
    struct DynamicBuffer;
    struct DescriptorPool;
    struct Program;

    void ProcessPendingReleases(bool force);

    static uint32_t AllocateDescriptor(DescriptorPool& pool);
    static void FreeDescriptor(DescriptorPool& pool, uint32_t descriptor);
    static Page* AllocatePage(PageAllocator& allocator);

    Page* AllocatePage(DynamicBuffer& buffer);
    void CreateBuffer(DynamicBuffer& buffer, uint32_t size, const char* label);
    void CreateBuffers();
    void DestroyBuffer(DynamicBuffer& buffer);
    void DestroyBuffers();
    void* MapBuffer(DynamicBuffer& buffer, uint32_t size);

    ID3D12RootSignature* CreateRootSignature(uint32_t flags);

    void CreateHeaps();
    void CreatePipelines(Program* program, uint8_t shader, const char* label,
        D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, bool stereo);
    void CreateShaders();
    void DestroyShaders();

    Noesis::Ptr<Noesis::RenderTarget> CreateRenderTarget(const char* label, uint32_t width,
        uint32_t height, ID3D12Resource* colorAA, ID3D12Resource* stencil);

    void FillCaps(bool sRGB);
    void InvalidateStateCache();

    void RebindRootSignatureDescriptors();
    void SetRootSignature(ID3D12RootSignature* rootSignature);
    void SetPipelineState(ID3D12PipelineState* state);
    void SetIndexBuffer(D3D12_GPU_VIRTUAL_ADDRESS address);
    void SetStencilRef(uint32_t stencilRef);

    void UploadTexture(uint32_t& index, Noesis::Texture* texture, uint32_t sampler);
    void UploadUniforms(uint32_t& index, uint32_t i, const Noesis::UniformData* data);
    void SetDescriptors(const Noesis::Batch& batch, uint32_t signature);
    void SetBuffers(const Noesis::Batch& batch, uint32_t stride);

private:
    ID3D12Device* mDevice;
    ID3D12Fence* mFrameFence;
    ID3D12GraphicsCommandList* mCommands;

#ifdef NS_PLATFORM_WINDOWS_DESKTOP
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
    WCHAR mPipelineCachePath[MAX_PATH];
    ID3D12PipelineLibrary* mLibrary;
    const void* mMappedLibrary;
    bool mLibraryUpdated;
#endif

    uint64_t mSafeFenceValue;

    DXGI_FORMAT mColorFormat;
    DXGI_FORMAT mStencilFormat;
    DXGI_SAMPLE_DESC mSampleDesc;

    Noesis::DeviceCaps mCaps;
    D3D12_HEAP_FLAGS mHeapFlags;

    struct Page
    {
        ID3D12Resource* buffer;
        void* base;
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
        UINT64 safeFenceValue;
        Page* next;
    };

    struct PageAllocator
    {
        struct Block
        {
            Page pages[16];
            uint32_t count;
            Block* next;
        };

        Block* blocks = nullptr;
    };

    struct DynamicBuffer
    {
        uint32_t size;
        uint32_t pos;
        uint32_t drawPos;

        const char* label;

        Page* currentPage;
        Page* freePages;
        Page* pendingPages;

        PageAllocator allocator;

        uint32_t numPages;
    };

    DynamicBuffer mVertices;
    DynamicBuffer mIndices;
    DynamicBuffer mConstants[4];
    DynamicBuffer mTexUpload;

    typedef Noesis::HashMap<uint32_t, ID3D12RootSignature*> RootSignatures;
    RootSignatures mRootSignatures;

    struct Program
    {
        ID3D12PipelineState* pso[256];
        ID3D12RootSignature* rootSignature;
        uint32_t signature;
        uint32_t stride;
    };

    Program mPrograms[Noesis::Shader::Count];
    Program mProgramsVR[Noesis::Shader::Count];

    Noesis::Vector<Program> mCustomShaders;
    Noesis::Vector<Program> mCustomShadersVR;

    ID3D12DescriptorHeap* mRTVHeap;
    ID3D12DescriptorHeap* mDSVHeap;
    ID3D12DescriptorHeap* mSRVHeap;
    ID3D12DescriptorHeap* mSamplerHeap;

    SIZE_T mRTVHeapStart;
    SIZE_T mDSVHeapStart;
    SIZE_T mSRVHeapStartCPU;
    UINT64 mSRVHeapStartGPU;
    UINT64 mSamplerHeapStartGPU;

    SIZE_T mRTVSize;
    SIZE_T mDSVSize;
    SIZE_T mSRVSize;
    UINT64 mSamplerSize;

    struct DescriptorPool
    {
        uint16_t nextDescriptor = 0;
        Noesis::Vector<uint16_t> freeDescriptors;
    };

    DescriptorPool mRTVPool;
    DescriptorPool mDSVPool;
    DescriptorPool mSRVPool;

    struct PendingRelease
    {
        uint32_t srv;
        uint32_t rtv;
        uint32_t dsv;
        uint64_t safeFenceValue;
        ID3D12Resource* object;
    };

    Noesis::Vector<PendingRelease> mPendingReleases;
    Noesis::Vector<Noesis::Texture*> mPendingSplitBarriers;

    ID3D12RootSignature* mCachedRootSignature;
    ID3D12PipelineState* mCachedPipelineState;
    D3D12_GPU_VIRTUAL_ADDRESS mCachedIndexBuffer;
    uint32_t mCachedStencilRef;
    uint32_t mCachedConstantHash[4];
    UINT64 mCachedConstantAddress[4];
    UINT64 mCachedRootSignatureParameters[10];
};

}

#endif
