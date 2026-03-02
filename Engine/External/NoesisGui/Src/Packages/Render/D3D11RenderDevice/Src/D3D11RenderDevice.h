////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __RENDER_D3D11RENDERDEVICE_H__
#define __RENDER_D3D11RENDERDEVICE_H__


#include <NsRender/RenderDevice.h>
#include <NsCore/Vector.h>


#if defined(NS_PLATFORM_WINDOWS_DESKTOP) || defined(NS_PLATFORM_WINRT)
    #include <d3d11_1.h>
#endif

#ifdef NS_PLATFORM_XBOX_ONE
    #ifndef NS_PROFILE
        #define D3DCOMPILE_NO_DEBUG_AND_ALL_FAST_SEMANTICS 1
    #endif
    #include <d3d11_x.h>
#endif


namespace Noesis { template<class T> class Ptr; }

namespace NoesisApp
{

struct MSAA
{
    enum Enum
    {
        x1,
        x2,
        x4,
        x8,
        x16,

        Count
    };
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// D3D11RenderDevice
////////////////////////////////////////////////////////////////////////////////////////////////////
class D3D11RenderDevice final: public Noesis::RenderDevice
{
public:
    D3D11RenderDevice(ID3D11DeviceContext* context, bool sRGB = false);
    ~D3D11RenderDevice();

    // Creates a Noesis texture from a D3D11 texture. Reference count is incremented by one
    static Noesis::Ptr<Noesis::Texture> WrapTexture(ID3D11Texture2D* texture, uint32_t width,
        uint32_t height, uint32_t levels, bool isInverted, bool hasAlpha);

    // Enables the hardware scissor rectangle
    void EnableScissorRect();

    // Disables the hardware scissor rectangle
    void DisableScissorRect();

    void* CreatePixelShader(const char* label, const void* hlsl, uint32_t size);
    void ClearPixelShaders();

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
    struct DynamicBuffer;

    void CreateBuffer(DynamicBuffer& buffer, uint32_t size, D3D11_BIND_FLAG flags, const char* label);
    void CreateBuffers();
    void DestroyBuffers();
    void* MapBuffer(DynamicBuffer& buffer, uint32_t size);
    void UnmapBuffer(DynamicBuffer& buffer) const;

    ID3D11InputLayout* CreateLayout(uint32_t format, const void* code, uint32_t size);
    void CreateStateObjects();
    void DestroyStateObjects();
    void CreateShaders();
    void DestroyShaders();
    Noesis::Ptr<Noesis::RenderTarget> CreateRenderTarget(const char* label, uint32_t width,
        uint32_t height, MSAA::Enum msaa, ID3D11Texture2D* colorAA, ID3D11Texture2D* stencil);

    void CheckMultisample();
    void FillCaps(bool sRGB);
    void InvalidateStateCache();

    void SetIndexBuffer(ID3D11Buffer* buffer);
    void SetVertexBuffer(ID3D11Buffer* buffer, uint32_t stride, uint32_t offset);
    void SetInputLayout(ID3D11InputLayout* layout);
    void SetVertexShader(ID3D11VertexShader* shader);
    void SetPixelShader(ID3D11PixelShader* shader);
    void SetRasterizerState(ID3D11RasterizerState* state);
    void SetBlendState(ID3D11BlendState* state);
    void SetDepthStencilState(ID3D11DepthStencilState* state, unsigned int stencilRef);
    void SetSampler(unsigned int slot, ID3D11SamplerState* sampler);
    void SetTexture(unsigned int slot, ID3D11ShaderResourceView* texture);
    void ClearTextures();

    void SetBuffers(const Noesis::Batch& batch, uint32_t stride);
    void SetRenderState(const Noesis::Batch& batch);
    void SetTextures(const Noesis::Batch& batch);

private:
    ID3D11Device* mDevice;
    ID3D11DeviceContext* mContext;
    ID3DUserDefinedAnnotation* mGroupMarker;

    bool mRenderTargetArrayIndexSupported;
    DXGI_SAMPLE_DESC mSampleDescs[MSAA::Count];
    Noesis::DeviceCaps mCaps;

    struct DynamicBuffer
    {
        uint32_t size;
        uint32_t pos;
        uint32_t drawPos;
        D3D11_BIND_FLAG flags;

        ID3D11Buffer* obj;
    };

    DynamicBuffer mVertices;
    DynamicBuffer mIndices;

    DynamicBuffer mVertexCB[2];
    uint32_t mVertexCBHash[2];

    DynamicBuffer mPixelCB[2];
    uint32_t mPixelCBHash[2];

    ID3D11InputLayout* mLayouts[Noesis::Shader::Vertex::Format::Count];

    struct VertexShader
    {
        ID3D11VertexShader* shader;
        ID3D11InputLayout* layout;
        uint32_t stride;
    };

    VertexShader mVertexShaders[Noesis::Shader::Vertex::Count];
    VertexShader mVertexShadersVR[Noesis::Shader::Vertex::Count];

    struct PixelShader
    {
        ID3D11PixelShader* shader;
        int8_t vsShader;
    };

    PixelShader mPixelShaders[Noesis::Shader::Count];

    Noesis::Vector<ID3D11PixelShader*> mCustomShaders;
    ID3D11PixelShader* mResolvePS[MSAA::Count - 1];
    ID3D11VertexShader* mQuadVS;

    uint32_t mScissorStateIndex;

    ID3D11RasterizerState* mRasterizerStates[4];
    ID3D11BlendState* mBlendStates[Noesis::BlendMode::Count];
    ID3D11BlendState* mBlendStateNoColor;
    ID3D11DepthStencilState* mDepthStencilStates[Noesis::StencilMode::Count];
    ID3D11SamplerState* mSamplerStates[64];

    struct TextureSlot
    {
        enum Enum
        {
            Pattern,
            Ramps,
            Image,
            Glyphs,
            Shadow,

            Count
        };
    };

    //Cached state
    //@{
    ID3D11Buffer* mIndexBuffer;
    ID3D11InputLayout* mLayout;
    ID3D11VertexShader* mVertexShader;
    ID3D11PixelShader* mPixelShader;
    ID3D11RasterizerState* mRasterizerState;
    ID3D11BlendState* mBlendState;
    ID3D11DepthStencilState* mDepthStencilState;
    unsigned int mStencilRef;
    ID3D11ShaderResourceView* mTextures[TextureSlot::Count];
    ID3D11SamplerState* mSamplers[TextureSlot::Count];
    //@}
};

}

#endif
