////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "D3D11MediaPlayer.h"

#ifdef NS_PLATFORM_WINDOWS_DESKTOP

#include <NsCore/HighResTimer.h>
#include <NsCore/UTF8.h>
#include <NsApp/MediaElement.h>
#include <NsGui/IntegrationAPI.h>
#include <NsGui/IView.h>
#include <NsGui/DynamicTextureSource.h>
#include <NsGui/Stream.h>
#include <NsRender/Texture.h>
#include <NsRender/RenderTarget.h>
#include <NsRender/RenderDevice.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfreadwrite.h>

#include <mmdeviceapi.h>
#include <Audioclient.h>

#include <d3d11.h>

#include "VideoVS.h"
#include "VideoPS.h"

// Declaration to avoid including d3d11_4.h.
MIDL_INTERFACE("9B7E4E00-342C-4106-A19F-4F2704F689F0")
ID3D11Multithread : public IUnknown
{
public:
    virtual void STDMETHODCALLTYPE Enter(void) = 0;
    virtual void STDMETHODCALLTYPE Leave(void) = 0;
    virtual BOOL STDMETHODCALLTYPE SetMultithreadProtected(_In_  BOOL bMTProtect) = 0;
    virtual BOOL STDMETHODCALLTYPE GetMultithreadProtected(void) = 0;
};

using namespace Noesis;
using namespace NoesisApp;

#define V(exp) \
    NS_MACRO_BEGIN \
        HRESULT hr_ = (exp); \
        NS_ASSERT(!FAILED(hr_)); \
    NS_MACRO_END

// mfplat.dll functions
typedef HRESULT(__stdcall* PFN_MF_STARTUP)(ULONG Version, DWORD dwFlags);
typedef HRESULT(__stdcall* PFN_MF_SHUTDOWN)();
typedef HRESULT(__stdcall* PFN_MF_CREATE_DXGI_DEVICE_MANAGER)(UINT* resetToken, IMFDXGIDeviceManager** ppDeviceManager);
typedef HRESULT(__stdcall* PFN_MF_CREATE_ATTRIBUTES)(IMFAttributes** ppMFAttributes, UINT32 cInitialSize);
typedef HRESULT(__stdcall* PFN_MF_CREATE_MEDIA_TYPE)(IMFMediaType** ppMFType);
typedef HRESULT(__stdcall* PFN_MF_CREATE_MF_BYTE_STREAM_ON_STREAM)(IStream * pStream, IMFByteStream** ppByteStream);
typedef HRESULT(__stdcall* PFN_MF_CREATE_WAVE_FORMAT_EX_FROM_MF_MEDIA_TYPE)(IMFMediaType* pMFType, WAVEFORMATEX** ppWF, UINT32* pcbSize, UINT32 Flags);

// mfreadwrite.dll functions
typedef HRESULT(__stdcall* PFN_MF_CREATE_SOURCE_READER_FROM_BYTE_STREAM)(IMFByteStream* pByteStream, IMFAttributes* pAttributes, IMFSourceReader** ppSourceReader);

static IMFDXGIDeviceManager* gDeviceManager;

static IMMDeviceEnumerator* gAudioEnumerator;
static IMMDevice* gAudioDevice;

static ID3D11Device* gD3D11Device;
static ID3D11DeviceContext* gD3D11DeviceContext;
static ID3D11VertexShader* gD3D11VertexShader;
static ID3D11PixelShader* gD3D11PixelShader;
static ID3D11RasterizerState* gD3D11RasterizerState;
static ID3D11BlendState* gD3D11BlendState;
static ID3D11DepthStencilState* gD3D11DepthStencilState;
static ID3D11SamplerState* gD3D11SamplerState;

static ID3D11Device* gD3D11MediaPlayerDevice;
static ID3D11DeviceContext* gD3D11MediaPlayerDeviceContext;

static PFN_D3D11_CREATE_DEVICE D3D11CreateDevice_;

static PFN_MF_STARTUP MFStartup_;
static PFN_MF_SHUTDOWN MFShutdown_;
static PFN_MF_CREATE_DXGI_DEVICE_MANAGER MFCreateDXGIDeviceManager_;
static PFN_MF_CREATE_ATTRIBUTES MFCreateAttributes_;
static PFN_MF_CREATE_MEDIA_TYPE MFCreateMediaType_;
static PFN_MF_CREATE_MF_BYTE_STREAM_ON_STREAM MFCreateMFByteStreamOnStream_;
static PFN_MF_CREATE_WAVE_FORMAT_EX_FROM_MF_MEDIA_TYPE MFCreateWaveFormatExFromMFMediaType_;

static PFN_MF_CREATE_SOURCE_READER_FROM_BYTE_STREAM MFCreateSourceReaderFromByteStream_;

static const GUID MFMediaType_Video_ = { 0x73646976, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };
static const GUID MFMediaType_Audio_ = { 0x73647561, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };
static const GUID MFVideoFormat_NV12_ = { FCC('NV12'), 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const GUID MFAudioFormat_Float_ = { WAVE_FORMAT_IEEE_FLOAT, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const GUID MF_MT_MAJOR_TYPE_ = { 0x48eba18e, 0xf8c9, 0x4687, { 0xbf, 0x11, 0x0a, 0x74, 0xc9, 0xf9, 0x6a, 0x8f } };
static const GUID MF_MT_SUBTYPE_ = { 0xf7e34c9a, 0x42e8, 0x4714, { 0xb7, 0x4b, 0xcb, 0x29, 0xd7, 0x2c, 0x35, 0xe5 } };
static const GUID MF_MT_FRAME_SIZE_ = { 0x1652c33d, 0xd6b2, 0x4012, { 0xb8, 0x34, 0x72, 0x03, 0x08, 0x49, 0xa3, 0x7d } };

////////////////////////////////////////////////////////////////////////////////////////////////////
class ProviderStream: public IStream
{
public:
    ProviderStream(Stream* stream): mStream(stream)
    {
    }

    virtual ~ProviderStream()
    {
    }

    // From IUnknown
    //@{
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override
    {
        *ppvObj = nullptr;

        if (IsEqualIID(riid, __uuidof(IUnknown)) || IsEqualIID(riid, __uuidof(IStream)))
        {
            *ppvObj = static_cast<IStream*>(this);
        }

        if (*ppvObj != 0)
        {
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override
    {
        return ++mRefs;
    }

    STDMETHODIMP_(ULONG) Release() override
    {
        ULONG newRefs;

        newRefs = --mRefs;

        if (newRefs == 0)
        {
            delete this;
        }

        return newRefs;
    }
    //@}

    // From IStream
    //@{
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) override
    {
        uint64_t position;
        if (dwOrigin == STREAM_SEEK_SET)
        {
            position = dlibMove.QuadPart;
        }
        else if (dwOrigin == STREAM_SEEK_CUR)
        {
            position = mStream->GetPosition() + dlibMove.QuadPart;
        }
        else if (dwOrigin == STREAM_SEEK_END)
        {
            position = mStream->GetLength() + dlibMove.QuadPart;
        }
        else
        {
            return STG_E_INVALIDFUNCTION;
        }

        NS_ASSERT(position < 0xffffffff);
        mStream->SetPosition((uint32_t)position);

        if (plibNewPosition != nullptr)
        {
            plibNewPosition->QuadPart = position;
        }

        return S_OK;
    }

    STDMETHODIMP SetSize(ULARGE_INTEGER /*libNewSize*/) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP CopyTo(IStream* /*pstm*/, ULARGE_INTEGER /*cb*/, ULARGE_INTEGER* /*pcbRead*/, ULARGE_INTEGER* /*pcbWritten*/) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP Commit(DWORD /*grfCommitFlags*/) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP  Revert() override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP  LockRegion(ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/, DWORD /*dwLockType*/) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP  UnlockRegion(ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/, DWORD /*dwLockType*/) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP  Stat(STATSTG* pstatstg, DWORD /*grfStatFlag*/) override
    {
        ZeroMemory(pstatstg, sizeof(*pstatstg));
        pstatstg->pwcsName = NULL;
        pstatstg->type = STGTY_STREAM;
        pstatstg->cbSize.QuadPart = (uint64_t)mStream->GetLength();

        return S_OK;
    }

    STDMETHODIMP  Clone(IStream** /*ppstm*/) override
    {
        return E_NOTIMPL;
    }
    //@}

    // From ISequentialStream
    //@{
    STDMETHODIMP Read(void* pv, ULONG cb, ULONG* pcbRead) override
    {
        uint32_t bytesRead = mStream->Read(pv, cb);

        if (pcbRead != nullptr)
        {
            *pcbRead = bytesRead;
        }

        return S_OK;
    }

    STDMETHODIMP Write(const void* /*pv*/, ULONG /*cb*/, ULONG* /*pcbWritten*/) override
    {
        return E_NOTIMPL;
    }
    //@}

private:
    Ptr<Stream> mStream = nullptr;
    ULONG mRefs = 1;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11MediaPlayer::Init()
{
    HMODULE d3dLib = LoadLibraryA("d3d11.dll");
    if (d3dLib != nullptr)
    {
        D3D11CreateDevice_ = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3dLib, "D3D11CreateDevice");
    }
    HMODULE mfplatLib = LoadLibraryA("mfplat.dll");
    if (mfplatLib != nullptr)
    {
        MFStartup_ = (PFN_MF_STARTUP)GetProcAddress(mfplatLib, "MFStartup");
        MFShutdown_ = (PFN_MF_SHUTDOWN)GetProcAddress(mfplatLib, "MFShutdown");
        MFCreateDXGIDeviceManager_ = (PFN_MF_CREATE_DXGI_DEVICE_MANAGER)GetProcAddress(mfplatLib, "MFCreateDXGIDeviceManager");
        MFCreateAttributes_ = (PFN_MF_CREATE_ATTRIBUTES)GetProcAddress(mfplatLib, "MFCreateAttributes");
        MFCreateMediaType_ = (PFN_MF_CREATE_MEDIA_TYPE)GetProcAddress(mfplatLib, "MFCreateMediaType");
        MFCreateMFByteStreamOnStream_ = (PFN_MF_CREATE_MF_BYTE_STREAM_ON_STREAM)GetProcAddress(mfplatLib, "MFCreateMFByteStreamOnStream");
        MFCreateWaveFormatExFromMFMediaType_ = (PFN_MF_CREATE_WAVE_FORMAT_EX_FROM_MF_MEDIA_TYPE)GetProcAddress(mfplatLib, "MFCreateWaveFormatExFromMFMediaType");
    }
    HMODULE mfreadwriteLib = LoadLibraryA("mfreadwrite.dll");
    if (mfreadwriteLib != nullptr)
    {
        MFCreateSourceReaderFromByteStream_ = (PFN_MF_CREATE_SOURCE_READER_FROM_BYTE_STREAM)GetProcAddress(mfreadwriteLib, "MFCreateSourceReaderFromByteStream");
    }
    if (MFStartup_ != nullptr)
    {
        V(MFStartup_(MF_VERSION, MFSTARTUP_NOSOCKET));
        V(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&gAudioEnumerator)));
        gAudioEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &gAudioDevice);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11MediaPlayer::Shutdown()
{
    if (gD3D11VertexShader != nullptr)
    {
        gD3D11VertexShader->Release();
    }

    if (gD3D11PixelShader != nullptr)
    {
        gD3D11PixelShader->Release();
    }

    if (gD3D11RasterizerState != nullptr)
    {
        gD3D11RasterizerState->Release();
    }

    if (gD3D11BlendState != nullptr)
    {
        gD3D11BlendState->Release();
    }

    if (gD3D11DepthStencilState != nullptr)
    {
        gD3D11DepthStencilState->Release();
    }

    if (gD3D11SamplerState != nullptr)
    {
        gD3D11SamplerState->Release();
    }

    if (gD3D11DeviceContext != nullptr)
    {
        gD3D11DeviceContext->Release();
    }

    if (gD3D11MediaPlayerDeviceContext != nullptr)
    {
        gD3D11MediaPlayerDeviceContext->Release();
    }

    if (gD3D11MediaPlayerDevice != nullptr)
    {
        gD3D11MediaPlayerDevice->Release();
    }

    if (gDeviceManager != nullptr)
    {
        gDeviceManager->Release();
    }

    if (gAudioDevice != nullptr)
    {
        gAudioDevice->Release();
    }

    if (gAudioEnumerator != nullptr)
    {
        gAudioEnumerator->Release();
    }

    if (MFShutdown_ != nullptr)
    {
        V(MFShutdown_());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<MediaPlayer> D3D11MediaPlayer::Create(MediaElement* owner, const Uri& uri, void* user)
{
    return *new D3D11MediaPlayer(owner, uri, user);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D11MediaPlayer::D3D11MediaPlayer(MediaElement* owner, const Uri& uri, void* user)
{
    if (MFStartup_ == nullptr)
    {
        return;
    }

    Ptr<Stream> sourceStream = GUI::LoadXamlResource(uri);
    if (sourceStream == nullptr)
    {
        return;
    }

    // If we create this D3D device in Init we get a crash during its destruction.
    if (gDeviceManager == nullptr)
    {
        UINT flags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT;

#ifdef NS_DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL features[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1,
        };

        D3D_FEATURE_LEVEL level;

        V(D3D11CreateDevice_(0, D3D_DRIVER_TYPE_HARDWARE, 0, flags, features,
            NS_COUNTOF(features), D3D11_SDK_VERSION, &gD3D11MediaPlayerDevice, &level, &gD3D11MediaPlayerDeviceContext));

        ID3D11Multithread* mt;
        gD3D11MediaPlayerDevice->QueryInterface(IID_PPV_ARGS(&mt));
        mt->SetMultithreadProtected(TRUE);
        mt->Release();

        // Create a DXGIDeviceManager and bind it to the D3D11 device
        uint32_t resetToken;
        V(MFCreateDXGIDeviceManager_(&resetToken, &gDeviceManager));
        V(gDeviceManager->ResetDevice(gD3D11MediaPlayerDevice, resetToken));

        // Get ahold of the D3D device and immediate context.
        gD3D11Device = (ID3D11Device*)user;
    }

    // Create the byte stream
    ProviderStream* providerStream = new ProviderStream(sourceStream);
    IMFByteStream* stream;
    V(MFCreateMFByteStreamOnStream_(providerStream, &stream));
    providerStream->Release();

    // Create the source reader
    IUnknown* deviceManagerUnknown;
    V(gDeviceManager->QueryInterface(IID_PPV_ARGS(&deviceManagerUnknown)));
    IMFAttributes* attributes;
    V(MFCreateAttributes_(&attributes, 3));
    V(attributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, deviceManagerUnknown));
    V(attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE));
    V(attributes->SetUINT32(MF_SOURCE_READER_DISABLE_DXVA, FALSE));
    V(MFCreateSourceReaderFromByteStream_(stream, attributes, &mReader));
    attributes->Release();
    deviceManagerUnknown->Release();
    stream->Release();

    mReader->SetStreamSelection((uint32_t)MF_SOURCE_READER_ALL_STREAMS, FALSE);

    // Set video stream subtype to NV12. RGB is less performant.
    // We'll convert the color space in a pixel shader
    IMFMediaType* nativeVideoType;
    if (SUCCEEDED(mReader->GetNativeMediaType((uint32_t)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &nativeVideoType)))
    {
        IMFMediaType* videoType;
        V(MFCreateMediaType_(&videoType));
        V(videoType->SetGUID(MF_MT_MAJOR_TYPE_, MFMediaType_Video_));
        V(videoType->SetGUID(MF_MT_SUBTYPE_, MFVideoFormat_NV12_));
        V(mReader->SetCurrentMediaType((uint32_t)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, videoType));
        videoType->Release();
        nativeVideoType->Release();
        mReader->SetStreamSelection((uint32_t)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
        mHasVideo = true;
    }

    // Set audio stream subtype to float.
    IMFMediaType* nativeAudioType;
    if (gAudioDevice != nullptr &&
        SUCCEEDED(mReader->GetNativeMediaType((uint32_t)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &nativeAudioType)))
    {
        IMFMediaType* audioType;
        V(MFCreateMediaType_(&audioType));
        V(audioType->SetGUID(MF_MT_MAJOR_TYPE_, MFMediaType_Audio_));
        V(audioType->SetGUID(MF_MT_SUBTYPE_, MFAudioFormat_Float_));
        V(mReader->SetCurrentMediaType((uint32_t)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, audioType));
        audioType->Release();
        nativeAudioType->Release();
        mReader->SetStreamSelection((uint32_t)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
        mHasAudio = true;
    }

    if (mHasVideo)
    {
        // Get the video dimensions.
        IMFMediaType* currentVideoType;
        V(mReader->GetCurrentMediaType((uint32_t)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &currentVideoType));
        V(MFGetAttributeSize(currentVideoType, MF_MT_FRAME_SIZE_, &mWidth, &mHeight));
        currentVideoType->Release();

        // TODO: We only need this texture until we figure out why the allocator attributes are not being
        // honored
        D3D11_TEXTURE2D_DESC textureDesc;
        textureDesc.Width = mWidth;
        textureDesc.Height = mHeight;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_NV12;
        textureDesc.SampleDesc = { 1, 0 };
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
        V(gD3D11MediaPlayerDevice->CreateTexture2D(&textureDesc, nullptr, &mD3D11MediaPlayerVideoTexture));

        mTextureSource = *new DynamicTextureSource(mWidth, mHeight, &D3D11MediaPlayer::TextureRenderCallback, this);
    }

    if (mHasAudio)
    {
        // Get the audio format.
        IMFMediaType* currentAudioType;
        V(mReader->GetCurrentMediaType((uint32_t)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &currentAudioType));
        uint32_t size;
        WAVEFORMATEX* streamFormat;
        MFCreateWaveFormatExFromMFMediaType_(currentAudioType, &streamFormat, &size, MFWaveFormatExConvertFlag_Normal);
        currentAudioType->Release();

        // Create the audio renderer.
        V(gAudioDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&mAudioClient));
        HRESULT formatSupported = mAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, streamFormat, &mWaveFormat);
        if (SUCCEEDED(formatSupported))
        {
            if (mWaveFormat == nullptr)
            {
                mWaveFormat = streamFormat;
            }
            else
            {
                CoTaskMemFree(streamFormat);
            }
            uint32_t bufferSize = 10000000; // one second
            V(mAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_RATEADJUST, bufferSize, 0, mWaveFormat, nullptr));
            V(mAudioClient->GetService(IID_PPV_ARGS(&mAudioRenderClient)));

            // Set the audio sample rate.
            IAudioClockAdjustment* audioClockAdjustment;
            V(mAudioClient->GetService(IID_PPV_ARGS(&audioClockAdjustment)));
            V(audioClockAdjustment->SetSampleRate((float)mWaveFormat->nSamplesPerSec));
            audioClockAdjustment->Release();
        }
        else
        {
            mHasAudio = false;
        }
    }
    if (!mHasAudio)
    {
        uint64_t currentTicks = HighResTimer::Ticks();
        mLastAudioTimestamp = (int64_t)currentTicks;
    }

    IView* view = owner->GetView();
    if (view != nullptr)
    {
        mView = view;
        view->Rendering() += MakeDelegate(this, &D3D11MediaPlayer::OnRendering);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ImageSource* D3D11MediaPlayer::GetTextureSource() const
{
    return mTextureSource;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t D3D11MediaPlayer::GetWidth() const
{
    return mWidth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t D3D11MediaPlayer::GetHeight() const
{
    return mHeight;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3D11MediaPlayer::CanPause() const
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3D11MediaPlayer::HasAudio() const
{
    return mHasAudio;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3D11MediaPlayer::HasVideo() const
{
    return mHasVideo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float D3D11MediaPlayer::GetBufferingProgress() const
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float D3D11MediaPlayer::GetDownloadProgress() const
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
double D3D11MediaPlayer::GetDuration() const
{
    PROPVARIANT var;
    V(mReader->GetPresentationAttribute((DWORD)MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var));
    NS_ASSERT(var.vt == VT_UI8);
    return (double)var.uhVal.QuadPart / 1e7;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
double D3D11MediaPlayer::GetPosition() const
{
    int64_t currentTimestamp = GetCurrentTimestamp();
    return (double)currentTimestamp / 1e7;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11MediaPlayer::SetPosition(double position)
{
    if (mReader != nullptr)
    {
        PROPVARIANT time;
        ZeroMemory(&time, sizeof(time));
        time.vt = VT_I8;
        time.hVal.QuadPart = (int64_t)(position * 1e7);
        V(mReader->SetCurrentPosition(GUID_NULL, time));
        V(mReader->Flush((DWORD)MF_SOURCE_READER_ALL_STREAMS));
        if (mAudioSample != nullptr)
        {
            mAudioSample->Release();
            mAudioSample = nullptr;
        }
        if (mVideoSample != nullptr)
        {
            mVideoSample->Release();
            mVideoSample = nullptr;
        }

        if (mHasAudio)
        {
            if (mIsPlaying)
            {
                V(mAudioClient->Stop());
            }
            V(mAudioClient->Reset());
            if (mIsPlaying)
            {
                V(mAudioClient->Start());
            }
        }
        else
        {
            mCurrentTimestamp = (uint64_t)(position * 1e7);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float D3D11MediaPlayer::GetSpeedRatio() const
{
    return mSpeedRatio;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11MediaPlayer::SetSpeedRatio(float speedRatio)
{
    mSpeedRatio = speedRatio;

    if (mHasAudio)
    {
        // Set the audio sample rate.
        IAudioClockAdjustment* audioClockAdjustment;
        V(mAudioClient->GetService(IID_PPV_ARGS(&audioClockAdjustment)));
        V(audioClockAdjustment->SetSampleRate((float)mWaveFormat->nSamplesPerSec * mSpeedRatio));
        audioClockAdjustment->Release();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float D3D11MediaPlayer::GetVolume() const
{
    if (mHasAudio)
    {
        ISimpleAudioVolume* audioVolume;
        V(mAudioClient->GetService(IID_PPV_ARGS(&audioVolume)));
        float volume;
        V(audioVolume->GetMasterVolume(&volume));
        audioVolume->Release();
        return volume;
    }

    return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11MediaPlayer::SetVolume(float volume)
{
    if (mHasAudio)
    {
        ISimpleAudioVolume* audioVolume;
        V(mAudioClient->GetService(IID_PPV_ARGS(&audioVolume)));
        V(audioVolume->SetMasterVolume(volume, nullptr));
        audioVolume->Release();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float D3D11MediaPlayer::GetBalance() const
{
    return 0.5f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11MediaPlayer::SetBalance(float)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3D11MediaPlayer::GetIsMuted() const
{
    if (mHasAudio)
    {
        ISimpleAudioVolume* audioVolume;
        V(mAudioClient->GetService(IID_PPV_ARGS(&audioVolume)));
        BOOL muted;
        V(audioVolume->GetMute(&muted));
        audioVolume->Release();
        return (muted != FALSE);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11MediaPlayer::SetIsMuted(bool muted)
{
    if (mHasAudio)
    {
        ISimpleAudioVolume* audioVolume;
        V(mAudioClient->GetService(IID_PPV_ARGS(&audioVolume)));
        V(audioVolume->SetMute(muted ? TRUE : FALSE, nullptr));
        audioVolume->Release();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3D11MediaPlayer::GetScrubbingEnabled() const
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11MediaPlayer::SetScrubbingEnabled(bool)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11MediaPlayer::Play()
{
    if (mHasAudio)
    {
        mAudioClient->Start();
    }

    mIsPlaying = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11MediaPlayer::Pause()
{
    if (mHasAudio)
    {
        mAudioClient->Stop();
    }

    mIsPlaying = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11MediaPlayer::Stop()
{
    Pause();
    SetPosition(0.0);
    mForceRenderFrame = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D11MediaPlayer::~D3D11MediaPlayer()
{
    IMFSample* videoSample = (IMFSample*)InterlockedExchangePointer((void* volatile*)&mRenderVideoSample, nullptr);
    if (videoSample != nullptr)
    {
        videoSample->Release();
    }

    if (mAudioSample != nullptr)
    {
        mAudioSample->Release();
    }

    if (mVideoSample != nullptr)
    {
        mVideoSample->Release();
    }

    if (mAudioClient != nullptr)
    {
        mAudioClient->Release();
    }

    if (mAudioRenderClient != nullptr)
    {
        mAudioRenderClient->Release();
    }

    if (mWaveFormat != nullptr)
    {
        CoTaskMemFree(mWaveFormat);
    }

    if (mView != nullptr)
    {
        mView->Rendering() -= MakeDelegate(this, &D3D11MediaPlayer::OnRendering);
    }

    if (mD3D11VideoTextureLumaView != nullptr)
    {
        mD3D11VideoTextureLumaView->Release();
    }

    if (mD3D11VideoTextureChromaView != nullptr)
    {
        mD3D11VideoTextureChromaView->Release();
    }

    if (mD3D11VideoTexture != nullptr)
    {
        mD3D11VideoTexture->Release();
    }

    if (mReader != nullptr)
    {
        mReader->Release();
    }

    if (mD3D11MediaPlayerVideoTexture != nullptr)
    {
        mD3D11MediaPlayerVideoTexture->Release();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D11MediaPlayer::OnRendering(IView* /*view*/)
{
    HRESULT hr = S_OK;
    DWORD streamIndex = MAXDWORD;
    DWORD streamFlags = 0;
    int64_t audioTimestamp = 0;

    bool audioEnded = false;

    if (mHasAudio)
    {
        for (;;)
        {
            // We push as many audio frames as we can.
            if (mAudioSample == nullptr)
            {
                hr = mReader->ReadSample((uint32_t)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIndex, &streamFlags, &audioTimestamp, &mAudioSample);
                if (streamFlags == MF_SOURCE_READERF_ENDOFSTREAM && mIsPlaying)
                {
                    audioEnded = true;
                }
            }
            else
            {
                mAudioSample->GetSampleTime(&audioTimestamp);
            }

            if (SUCCEEDED(hr) && (mAudioSample != nullptr))
            {
                IMFMediaBuffer* buffer;
                V(mAudioSample->ConvertToContiguousBuffer(&buffer));

                DWORD bufferLength;
                V(buffer->GetCurrentLength(&bufferLength));

                uint32_t frameSize = mWaveFormat->nChannels * mWaveFormat->wBitsPerSample / 8;
                uint32_t numFrames = bufferLength / frameSize;

                uint8_t* outputBuffer;
                // If GetBuffer fails it means the buffer is full.
                if (SUCCEEDED(mAudioRenderClient->GetBuffer(numFrames, &outputBuffer)))
                {
                    uint8_t* audioData;
                    buffer->Lock(&audioData, nullptr, nullptr);
                    memcpy(outputBuffer, audioData, bufferLength);
                    buffer->Unlock();
                    mAudioRenderClient->ReleaseBuffer(numFrames, 0);
                    buffer->Release();
                    mAudioSample->GetSampleTime(&mLastAudioTimestamp);
                    mAudioSample->Release();
                    mAudioSample = nullptr;
                }
                else
                {
                    buffer->Release();
                    break;
                }
            }
            else
            {
                if (audioEnded)
                {
                    uint32_t audioBufferPadding;
                    mAudioClient->GetCurrentPadding(&audioBufferPadding);
                    if (audioBufferPadding == 0)
                    {
                        mHasMediaEnded = true;
                    }
                }

                break;
            }
        }
    }
    else
    {
        uint64_t currentTicks = HighResTimer::Ticks();
        uint64_t elapsedTicks = currentTicks - (uint64_t)mLastAudioTimestamp;
        mLastAudioTimestamp = (int64_t)currentTicks;
        if (mIsPlaying)
        {
            double elapsedTime = HighResTimer::Seconds(elapsedTicks) * mSpeedRatio * 1e7;
            mCurrentTimestamp += (uint64_t)elapsedTime;
            PROPVARIANT var;
            V(mReader->GetPresentationAttribute((DWORD)MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var));
            NS_ASSERT(var.vt == VT_UI8);
            if (mCurrentTimestamp > var.uhVal.QuadPart)
            {
                mCurrentTimestamp = var.uhVal.QuadPart;
                mHasMediaEnded = true;
            }
        }
    }

    audioTimestamp = GetCurrentTimestamp();

    if (mHasVideo)
    {
        int64_t videoTimestamp = 0;
        for (;;)
        {
            if (mVideoSample == nullptr)
            {
                hr = mReader->ReadSample((uint32_t)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &streamIndex, &streamFlags, &videoTimestamp, &mVideoSample);
                if (streamFlags == MF_SOURCE_READERF_ENDOFSTREAM && mIsPlaying)
                {
                    mHasMediaEnded = true;
                }
            }
            else
            {
                mVideoSample->GetSampleTime(&videoTimestamp);
            }

            if (mVideoSample == nullptr)
                break;

            // Skip samples until we catch up with the audio
            int64_t duration;
            mVideoSample->GetSampleDuration(&duration);
            if (videoTimestamp + duration > audioTimestamp)
                break;

            mVideoSample->Release();
            mVideoSample = nullptr;
        }

        if (SUCCEEDED(hr) && (mVideoSample != nullptr) && ((audioTimestamp >= videoTimestamp) || mForceRenderFrame))
        {
            IMFSample* prevRenderVideoSample = (IMFSample*)InterlockedExchangePointer((void* volatile*)&mRenderVideoSample, mVideoSample);
            if (prevRenderVideoSample != nullptr)
            {
                prevRenderVideoSample->Release();
            }
            mVideoSample = nullptr;
            mForceRenderFrame = false;
        }
    }

    if (!mMediaOpenedSent)
    {
        mMediaOpened.Invoke();
        mMediaOpenedSent = true;
    }

    if (mHasMediaEnded && (mVideoSample == nullptr))
    {
        mHasMediaEnded = false;
        Pause();
        mMediaEnded.Invoke();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Texture* D3D11MediaPlayer::TextureRenderCallback(RenderDevice* device, void* user)
{
    D3D11MediaPlayer* player = (D3D11MediaPlayer*)user;
    return player->TextureRender(device);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Texture* D3D11MediaPlayer::TextureRender(RenderDevice* device)
{
    if (gD3D11DeviceContext == nullptr)
    {
        gD3D11Device->GetImmediateContext(&gD3D11DeviceContext);

        // Create the D3D11 objects needed for the color space conversion
        V(gD3D11Device->CreateVertexShader(VideoVS, sizeof(VideoVS), nullptr, &gD3D11VertexShader));
        V(gD3D11Device->CreatePixelShader(VideoPS, sizeof(VideoPS), nullptr, &gD3D11PixelShader));

        D3D11_RASTERIZER_DESC rasterizerDesc;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.FrontCounterClockwise = false;
        rasterizerDesc.DepthBias = 0;
        rasterizerDesc.DepthBiasClamp = 0.0f;
        rasterizerDesc.SlopeScaledDepthBias = 0.0f;
        rasterizerDesc.DepthClipEnable = true;
        rasterizerDesc.MultisampleEnable = true;
        rasterizerDesc.AntialiasedLineEnable = false;
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.ScissorEnable = false;
        V(gD3D11Device->CreateRasterizerState(&rasterizerDesc, &gD3D11RasterizerState));

        D3D11_BLEND_DESC blendDesc;
        blendDesc.AlphaToCoverageEnable = false;
        blendDesc.IndependentBlendEnable = false;
        blendDesc.RenderTarget[0].BlendEnable = false;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        V(gD3D11Device->CreateBlendState(&blendDesc, &gD3D11BlendState));

        D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
        depthStencilDesc.DepthEnable = false;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_NEVER;
        depthStencilDesc.StencilReadMask = 0xff;
        depthStencilDesc.StencilWriteMask = 0xff;
        depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.StencilEnable = false;
        depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        V(gD3D11Device->CreateDepthStencilState(&depthStencilDesc, &gD3D11DepthStencilState));

        D3D11_SAMPLER_DESC samplerDesc;
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MipLODBias = 0;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.MinLOD = -D3D11_FLOAT32_MAX;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        V(gD3D11Device->CreateSamplerState(&samplerDesc, &gD3D11SamplerState));
    }

    if (mD3D11VideoTexture == nullptr)
    {
        IDXGIResource* sharedTexture;
        V(mD3D11MediaPlayerVideoTexture->QueryInterface(IID_PPV_ARGS(&sharedTexture)));
        HANDLE sharedHandle;
        sharedTexture->GetSharedHandle(&sharedHandle);
        sharedTexture->Release();

        gD3D11Device->OpenSharedResource(sharedHandle, IID_PPV_ARGS(&mD3D11VideoTexture));

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
        viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MipLevels = 1;
        viewDesc.Texture2D.MostDetailedMip = 0;
        viewDesc.Format = DXGI_FORMAT_R8_UNORM;
        V(gD3D11Device->CreateShaderResourceView(mD3D11VideoTexture, &viewDesc, &mD3D11VideoTextureLumaView));
        viewDesc.Format = DXGI_FORMAT_R8G8_UNORM;
        V(gD3D11Device->CreateShaderResourceView(mD3D11VideoTexture, &viewDesc, &mD3D11VideoTextureChromaView));
    }

    IMFSample* videoSample = (IMFSample*)InterlockedExchangePointer((void* volatile*)&mRenderVideoSample, nullptr);
    // Hold the video frame until the time is right
    if (videoSample != nullptr)
    {
        IMFMediaBuffer* buffer;
        V(videoSample->ConvertToContiguousBuffer(&buffer));

        IMFDXGIBuffer* dxgiBuffer;
        V(buffer->QueryInterface(&dxgiBuffer));

        ID3D11Texture2D* texture;
        V(dxgiBuffer->GetResource(IID_PPV_ARGS(&texture)));

        UINT32 subResourceIndex;
        V(dxgiBuffer->GetSubresourceIndex(&subResourceIndex));

        if (mRenderTarget == nullptr ||
            mRenderTarget->GetTexture()->GetWidth() != mWidth ||
            mRenderTarget->GetTexture()->GetHeight() != mHeight)
        {
            mRenderTarget = device->CreateRenderTarget("MediaPlayer", mWidth, mHeight, 1, false);
        }

        D3D11_BOX box{ 0, 0, 0, mWidth, mHeight, 1 };
        gD3D11MediaPlayerDeviceContext->CopySubresourceRegion(mD3D11MediaPlayerVideoTexture, 0, 0, 0, 0, texture, subResourceIndex, &box);
        gD3D11MediaPlayerDeviceContext->Flush();

        texture->Release();
        dxgiBuffer->Release();
        buffer->Release();
        videoSample->Release();

        device->SetRenderTarget(mRenderTarget);

        gD3D11DeviceContext->IASetInputLayout(nullptr);
        gD3D11DeviceContext->VSSetShader(gD3D11VertexShader, 0, 0);
        gD3D11DeviceContext->PSSetShader(gD3D11PixelShader, 0, 0);
        gD3D11DeviceContext->RSSetState(gD3D11RasterizerState);
        gD3D11DeviceContext->OMSetBlendState(gD3D11BlendState, nullptr, 0xffffffff);
        gD3D11DeviceContext->OMSetDepthStencilState(gD3D11DepthStencilState, 0);
        ID3D11ShaderResourceView* textures[] = { mD3D11VideoTextureLumaView, mD3D11VideoTextureChromaView };
        gD3D11DeviceContext->PSSetShaderResources(0, 2, textures);
        gD3D11DeviceContext->PSSetSamplers(0, 1, &gD3D11SamplerState);

        gD3D11DeviceContext->Draw(6, 0);

        Tile tile = { 0, 0, mWidth, mHeight };
        device->ResolveRenderTarget(mRenderTarget, &tile, 1);
    }

    return mRenderTarget != nullptr ? mRenderTarget->GetTexture() : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int64_t D3D11MediaPlayer::GetCurrentTimestamp() const
{
    if (mHasAudio)
    {
        // We need to account for the samples that have been queued but not yet played.
        uint32_t audioBufferPadding;
        mAudioClient->GetCurrentPadding(&audioBufferPadding);
        int64_t audioBufferHNS = (int64_t)audioBufferPadding * 10000000 / mWaveFormat->nSamplesPerSec;
        int64_t currentTimestamp = mLastAudioTimestamp - audioBufferHNS;
        if (currentTimestamp < 0)
        {
            currentTimestamp = 0;
        }
        return currentTimestamp;
    }
    else
    {
        return mCurrentTimestamp;
    }
}

#endif
