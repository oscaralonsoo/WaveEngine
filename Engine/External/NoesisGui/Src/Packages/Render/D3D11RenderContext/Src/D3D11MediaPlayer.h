////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __RENDER_D3D11MEDIAPLAYER_H__
#define __RENDER_D3D11MEDIAPLAYER_H__


#include <NsApp/MediaPlayer.h>


struct IMFDXGIDeviceManager;
struct IMFVideoSampleAllocatorEx;
struct IMFSourceReader;
struct IMFSample;

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11RasterizerState;
struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11SamplerState;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;

struct tWAVEFORMATEX;

struct IAudioClient;
struct IAudioRenderClient;

namespace Noesis
{
class ImageSource;
class Texture;
class RenderTarget;
class RenderDevice;
class DynamicTextureSource;
struct IView;
struct Uri;
template<class T> class Ptr;
}

namespace NoesisApp
{

class MediaElement;

////////////////////////////////////////////////////////////////////////////////////////////////////
class D3D11MediaPlayer: public MediaPlayer
{
public:
    // Static initialization and shutdown
    static void Init();
    static void Shutdown();

    // Creation callback
    static Noesis::Ptr<MediaPlayer> Create(MediaElement* owner, const Noesis::Uri& uri, void* user);

    /// From MediaPlayer
    //@{
    Noesis::ImageSource* GetTextureSource() const override;
    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;
    bool CanPause() const override;
    bool HasAudio() const override;
    bool HasVideo() const override;
    float GetBufferingProgress() const override;
    float GetDownloadProgress() const override;
    double GetDuration() const override;
    double GetPosition() const override;
    void SetPosition(double position) override;
    float GetSpeedRatio() const override;
    void SetSpeedRatio(float speedRatio) override;
    float GetVolume() const override;
    void SetVolume(float volume) override;
    float GetBalance() const override;
    void SetBalance(float balance) override;
    bool GetIsMuted() const override;
    void SetIsMuted(bool muted) override;
    bool GetScrubbingEnabled() const override;
    void SetScrubbingEnabled(bool scrubbing) override;
    void Play() override;
    void Pause() override;
    void Stop() override;
    //@}

private:
    D3D11MediaPlayer(MediaElement* owner, const Noesis::Uri& uri, void* user);
    ~D3D11MediaPlayer();

    void OnRendering(Noesis::IView* view);

    static Noesis::Texture* TextureRenderCallback(Noesis::RenderDevice* device, void* user);

    Noesis::Texture* TextureRender(Noesis::RenderDevice* device);

    int64_t GetCurrentTimestamp() const;

    IMFSourceReader* mReader = nullptr;
    IMFSample* mAudioSample = nullptr;
    IMFSample* mVideoSample = nullptr;

    IMFSample* mRenderVideoSample = nullptr;

    ID3D11Texture2D* mD3D11MediaPlayerVideoTexture = nullptr;
    ID3D11Texture2D* mD3D11VideoTexture = nullptr;
    ID3D11ShaderResourceView* mD3D11VideoTextureLumaView = nullptr;
    ID3D11ShaderResourceView* mD3D11VideoTextureChromaView = nullptr;

    tWAVEFORMATEX* mWaveFormat = nullptr;

    IAudioClient* mAudioClient = nullptr;
    IAudioRenderClient* mAudioRenderClient = nullptr;
    int64_t mLastAudioTimestamp = 0;

    Noesis::Ptr<Noesis::DynamicTextureSource> mTextureSource;
    Noesis::Ptr<Noesis::RenderTarget> mRenderTarget; 

    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    Noesis::IView* mView = nullptr;
    bool mMediaOpenedSent = false;
    bool mHasMediaEnded = false;
    bool mForceRenderFrame = true;
    bool mHasAudio = false;
    bool mHasVideo = false;
    float mSpeedRatio = 1.0f;

    uint64_t mCurrentTimestamp = 0;
    bool mIsPlaying = false;
};

}

#endif
