////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "D3D12MediaPlayer.h"
#include "D3D12RenderContext.h"
#include <NsRender/D3D12Factory.h>

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

#include "directx/d3d11_4.h"
#include "directx/d3d12.h"
#include "directx/d3d12video.h"
#include "directx/dxgi1_5.h"


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

static IMFDXGIDeviceManager* gDXGIDeviceManager;

static ID3D11Device5* gD3D11Device;
static ID3D11DeviceContext4* gD3D11Context;
static ID3D11Fence* gD3D11VideoTextureFence;

static IMMDeviceEnumerator* gAudioEnumerator;
static IMMDevice* gAudioDevice;

static ID3D12Device* gD3D12Device;
static ID3D12VideoDevice* gD3D12VideoDevice;
static ID3D12VideoProcessor* gD3D12VideoProcessor;
static ID3D12CommandQueue* gD3D12VideoCommandQueue;
static ID3D12Fence* gD3D12Fence;
static uint64_t gD3D12FenceValue;
static ID3D12Fence* gD3D12VideoTextureFence;
static uint64_t gVideoTextureFenceValue;

static D3D12RenderContext* gRenderContext;

static PFN_D3D11_CREATE_DEVICE D3D11CreateDevice_;

// d3d12.dll functions
static PFN_D3D12_CREATE_DEVICE D3D12CreateDevice_;
static PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface_;

// dxgi.dll functions
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT, REFIID, _COM_Outptr_ void**);
static PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2_;

// mfplat.dll functions
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
void D3D12MediaPlayer::Init()
{
    HMODULE d3d11Lib = LoadLibraryA("d3d11.dll");
    if (d3d11Lib != nullptr)
    {
        D3D11CreateDevice_ = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11Lib, "D3D11CreateDevice");
    }
    HMODULE d3dLib = LoadLibraryA("d3d12.dll");
    if (d3dLib != nullptr)
    {
        D3D12CreateDevice_ = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3dLib, "D3D12CreateDevice");
        D3D12GetDebugInterface_ = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3dLib, "D3D12GetDebugInterface");
    }
    HMODULE dxgiLib = LoadLibraryA("dxgi.dll");
    if (dxgiLib != 0)
    {
        CreateDXGIFactory2_ = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");
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
void D3D12MediaPlayer::Shutdown()
{
    if (gD3D11VideoTextureFence != nullptr)
    {
        gD3D11VideoTextureFence->Release();
    }

    if (gD3D11Context != nullptr)
    {
        gD3D11Context->Release();
    }

    if (gD3D11Device != nullptr)
    {
        gD3D11Device->Release();
    }

    if (gDXGIDeviceManager != nullptr)
    {
        gDXGIDeviceManager->Release();
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

    if (gD3D12VideoProcessor != nullptr)
    {
        gD3D12VideoProcessor->Release();
    }

    if (gD3D12VideoDevice != nullptr)
    {
        gD3D12VideoDevice->Release();
    }

    if (gD3D12VideoCommandQueue != nullptr)
    {
        gD3D12VideoCommandQueue->Release();
    }

    if (gD3D12Fence != nullptr)
    {
        gD3D12Fence->Release();
    }

    if (gD3D12VideoTextureFence != nullptr)
    {
        gD3D12VideoTextureFence->Release();
    }

    if (gD3D12Device != nullptr)
    {
        gD3D12Device->Release();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<MediaPlayer> D3D12MediaPlayer::Create(MediaElement* owner, const Uri& uri, void* user)
{
    return *new D3D12MediaPlayer(owner, uri, user);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D12MediaPlayer::D3D12MediaPlayer(MediaElement* owner, const Uri& uri, void* user)
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

    if (gDXGIDeviceManager == nullptr)
    {
        UINT flags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT;

#ifdef NS_DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL features[] = { D3D_FEATURE_LEVEL_12_0 };
        D3D_FEATURE_LEVEL level;

        ID3D11Device* d3d11Device;
        ID3D11DeviceContext* d3d11Context;
         V(D3D11CreateDevice_(0, D3D_DRIVER_TYPE_HARDWARE, 0, flags, features,
            NS_COUNTOF(features), D3D11_SDK_VERSION, &d3d11Device, &level, &d3d11Context));
 
        V(d3d11Device->QueryInterface(IID_PPV_ARGS(&gD3D11Device)));
        d3d11Device->Release();

        V(d3d11Context->QueryInterface(IID_PPV_ARGS(&gD3D11Context)));
        d3d11Context->Release();

        ID3D11Multithread* mt;
        gD3D11Device->QueryInterface(IID_PPV_ARGS(&mt));
        mt->SetMultithreadProtected(TRUE);
        mt->Release();

        V(gD3D11Device->CreateFence(0, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(&gD3D11VideoTextureFence)));

        // Create a DXGIDeviceManager and bind it to the D3D11 device
        uint32_t resetToken;
        V(MFCreateDXGIDeviceManager_(&resetToken, &gDXGIDeviceManager));
        V(gDXGIDeviceManager->ResetDevice(gD3D11Device, resetToken));

        gRenderContext = (D3D12RenderContext*)user;
    }

    
    // Create the byte stream
    ProviderStream* providerStream = new ProviderStream(sourceStream);
    IMFByteStream* stream;
    V(MFCreateMFByteStreamOnStream_(providerStream, &stream));
    providerStream->Release();

    // Create the source reader
    IUnknown* deviceManagerUnknown;
    V(gDXGIDeviceManager->QueryInterface(IID_PPV_ARGS(&deviceManagerUnknown)));
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
        V(gD3D11Device->CreateTexture2D(&textureDesc, nullptr, &mD3D11VideoTexture[0]));
        V(gD3D11Device->CreateTexture2D(&textureDesc, nullptr, &mD3D11VideoTexture[1]));

        mTextureSource = *new DynamicTextureSource(mWidth, mHeight, &D3D12MediaPlayer::TextureRenderCallback, this);
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
        view->Rendering() += MakeDelegate(this, &D3D12MediaPlayer::OnRendering);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ImageSource* D3D12MediaPlayer::GetTextureSource() const
{
    return mTextureSource;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t D3D12MediaPlayer::GetWidth() const
{
    return mWidth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t D3D12MediaPlayer::GetHeight() const
{
    return mHeight;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3D12MediaPlayer::CanPause() const
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3D12MediaPlayer::HasAudio() const
{
    return mHasAudio;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3D12MediaPlayer::HasVideo() const
{
    return mHasVideo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float D3D12MediaPlayer::GetBufferingProgress() const
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float D3D12MediaPlayer::GetDownloadProgress() const
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
double D3D12MediaPlayer::GetDuration() const
{
    PROPVARIANT var;
    V(mReader->GetPresentationAttribute((DWORD)MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var));
    NS_ASSERT(var.vt == VT_UI8);
    return (double)var.uhVal.QuadPart / 1e7;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
double D3D12MediaPlayer::GetPosition() const
{
    int64_t currentTimestamp = GetCurrentTimestamp();
    return (double)currentTimestamp / 1e7;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12MediaPlayer::SetPosition(double position)
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
float D3D12MediaPlayer::GetSpeedRatio() const
{
    return mSpeedRatio;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12MediaPlayer::SetSpeedRatio(float speedRatio)
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
float D3D12MediaPlayer::GetVolume() const
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
void D3D12MediaPlayer::SetVolume(float volume)
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
float D3D12MediaPlayer::GetBalance() const
{
    return 0.5f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12MediaPlayer::SetBalance(float)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3D12MediaPlayer::GetIsMuted() const
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
void D3D12MediaPlayer::SetIsMuted(bool muted)
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
bool D3D12MediaPlayer::GetScrubbingEnabled() const
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12MediaPlayer::SetScrubbingEnabled(bool)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12MediaPlayer::Play()
{
    if (mHasAudio)
    {
        mAudioClient->Start();
    }

    mIsPlaying = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12MediaPlayer::Pause()
{
    if (mHasAudio)
    {
        mAudioClient->Stop();
    }

    mIsPlaying = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12MediaPlayer::Stop()
{
    Pause();
    SetPosition(0.0);
    mForceRenderFrame = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
D3D12MediaPlayer::~D3D12MediaPlayer()
{
    if (gD3D12Fence != nullptr)
    {
        if (gD3D12Fence->GetCompletedValue() < mCommandAllocatorFenceValue[mFrameIndex])
        {
            HANDLE fenceEvent = CreateEvent(nullptr, false, false, nullptr);
            gD3D12Fence->SetEventOnCompletion(mCommandAllocatorFenceValue[mFrameIndex], fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
            CloseHandle(fenceEvent);
        }
    }

    if (mD3D12RGBATexture != nullptr)
    {
        mD3D12RGBATexture->Release();
    }

    if (mD3D12VideoCommandList != nullptr)
    {
        mD3D12VideoCommandList->Release();
    }

    if (mD3D12VideoCommandAllocator[0] != nullptr)
    {
        mD3D12VideoCommandAllocator[0]->Release();
    }

    if (mD3D12VideoCommandAllocator[1] != nullptr)
    {
        mD3D12VideoCommandAllocator[1]->Release();
    }

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
        mView->Rendering() -= MakeDelegate(this, &D3D12MediaPlayer::OnRendering);
    }

    if (mD3D12VideoTexture[0] != nullptr)
    {
        mD3D12VideoTexture[0]->Release();
    }

    if (mD3D12VideoTexture[1] != nullptr)
    {
        mD3D12VideoTexture[1]->Release();
    }

    if (mReader != nullptr)
    {
        mReader->Release();
    }

    if (mD3D11VideoTexture[0] != nullptr)
    {
        mD3D11VideoTexture[0]->Release();
    }

    if (mD3D11VideoTexture[1] != nullptr)
    {
        mD3D11VideoTexture[1]->Release();
    }

    if (mTexture != nullptr)
    {
        gRenderContext->mTransitionVideoTextures -= MakeDelegate(this, &D3D12MediaPlayer::TransitionVideoTexture);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12MediaPlayer::OnRendering(IView* /*view*/)
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
Texture* D3D12MediaPlayer::TextureRenderCallback(RenderDevice*, void* user)
{
    D3D12MediaPlayer* player = (D3D12MediaPlayer*)user;
    return player->TextureRender();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Texture* D3D12MediaPlayer::TextureRender()
{
    if (gD3D12Device == nullptr)
    {
        // Get ahold of the D3D device.
        gD3D12Device = gRenderContext->mDevice;
        gD3D12Device->AddRef();

        HANDLE videoTextureFenceHandle;
        gD3D11VideoTextureFence->CreateSharedHandle(nullptr, GENERIC_ALL, TEXT("NoesisVideoTextureFence"), &videoTextureFenceHandle); 
        gD3D12Device->OpenSharedHandle(videoTextureFenceHandle, IID_PPV_ARGS(&gD3D12VideoTextureFence));

        gD3D12Device->QueryInterface(IID_PPV_ARGS(&gD3D12VideoDevice));

        D3D12_VIDEO_PROCESS_INPUT_STREAM_DESC inputStreamDesc{};
        inputStreamDesc.Format = DXGI_FORMAT_NV12;
        inputStreamDesc.ColorSpace = DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709;
        inputStreamDesc.SourceSizeRange = D3D12_VIDEO_SIZE_RANGE{ 3840, 2160, 128, 72 };
        inputStreamDesc.DestinationSizeRange = D3D12_VIDEO_SIZE_RANGE{ 3840, 2160, 128, 72 };

        D3D12_VIDEO_PROCESS_OUTPUT_STREAM_DESC outputStreamDesc{};
        outputStreamDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        outputStreamDesc.ColorSpace = DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709;

        V(gD3D12VideoDevice->CreateVideoProcessor(0, &outputStreamDesc, 1, &inputStreamDesc, IID_PPV_ARGS(&gD3D12VideoProcessor)));

        D3D12_COMMAND_QUEUE_DESC descQueue{};
        descQueue.Type = D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS;
        descQueue.Priority = 0;
        descQueue.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        V(gD3D12Device->CreateCommandQueue(&descQueue, IID_PPV_ARGS(&gD3D12VideoCommandQueue)));
        gD3D12VideoCommandQueue->SetName(TEXT("MediaPlayer_CommandQueue"));

        V(gD3D12Device->CreateFence(++gD3D12FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gD3D12Fence)));
    }

    if (mD3D12VideoCommandList == nullptr)
    {
        IDXGIResource* sharedTexture;
        V(mD3D11VideoTexture[0]->QueryInterface(IID_PPV_ARGS(&sharedTexture)));
        HANDLE videoTexture0Handle;
        sharedTexture->GetSharedHandle(&videoTexture0Handle);
        sharedTexture->Release();

        V(mD3D11VideoTexture[1]->QueryInterface(IID_PPV_ARGS(&sharedTexture)));
        HANDLE videoTexture1Handle;
        sharedTexture->GetSharedHandle(&videoTexture1Handle);
        sharedTexture->Release();

        gD3D12Device->OpenSharedHandle(videoTexture0Handle, IID_PPV_ARGS(&mD3D12VideoTexture[0]));
        mD3D12VideoTexture[0]->SetName(TEXT("MediaPlayer_TEX0"));
        gD3D12Device->OpenSharedHandle(videoTexture1Handle, IID_PPV_ARGS(&mD3D12VideoTexture[1]));
        mD3D12VideoTexture[1]->SetName(TEXT("MediaPlayer_TEX1"));

        V(gD3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS, IID_PPV_ARGS(&mD3D12VideoCommandAllocator[0])));
        V(gD3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS, IID_PPV_ARGS(&mD3D12VideoCommandAllocator[1])));

        V(gD3D12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS, mD3D12VideoCommandAllocator[mFrameIndex], nullptr, __uuidof(ID3D12CommandList), (void**)&mD3D12VideoCommandList));
        mD3D12VideoCommandList->Close();
        mD3D12VideoCommandList->SetName(TEXT("MediaPlayer_CommandList"));
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

        if (mTexture == nullptr)
        {
            D3D12_RESOURCE_DESC desc;
            desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Alignment = 0;
            desc.Width = mWidth;
            desc.Height = mHeight;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc = { 1, 0 };
            desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;

            D3D12_HEAP_PROPERTIES heap;
            heap.Type = D3D12_HEAP_TYPE_DEFAULT;
            heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap.CreationNodeMask = 0;
            heap.VisibleNodeMask = 0;

            V(gD3D12Device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON,
                nullptr, IID_PPV_ARGS(&mD3D12RGBATexture)));
            mD3D12RGBATexture->SetName(TEXT("MediaPlayerRGB_TEX"));
            mTexture = D3D12Factory::WrapTexture(mD3D12RGBATexture, mWidth, mHeight, 1, false, false, D3D12_RESOURCE_STATE_COMMON);

            gRenderContext->mTransitionVideoTextures += MakeDelegate(this, &D3D12MediaPlayer::TransitionVideoTexture);
        }

        D3D11_BOX box{ 0, 0, 0, mWidth, mHeight, 1 };
        gD3D11Context->CopySubresourceRegion(mD3D11VideoTexture[subResourceIndex & 1], 0, 0, 0, 0, texture, subResourceIndex, &box);
        mVideoTextureFenceValue = ++gVideoTextureFenceValue;
        gD3D11Context->Signal(gD3D11VideoTextureFence, mVideoTextureFenceValue);
        gD3D11Context->Flush();

        texture->Release();
        dxgiBuffer->Release();
        buffer->Release();
        videoSample->Release();

        D3D12_VIDEO_PROCESS_INPUT_STREAM_ARGUMENTS inputArgs{
            {
                {
                    mD3D12VideoTexture[subResourceIndex & 1],
                    0,
                    D3D12_VIDEO_PROCESS_REFERENCE_SET{}
                },
                {
                    nullptr,
                    0,
                    D3D12_VIDEO_PROCESS_REFERENCE_SET{}
                },
            },
            D3D12_VIDEO_PROCESS_TRANSFORM { { 0 ,0, (LONG)mWidth, (LONG)mHeight },
                                        { 0 ,0, (LONG)mWidth, (LONG)mHeight } },
            D3D12_VIDEO_PROCESS_INPUT_STREAM_FLAG_NONE,
            D3D12_VIDEO_PROCESS_INPUT_STREAM_RATE {},
            {}, // Filter Levels
            D3D12_VIDEO_PROCESS_ALPHA_BLENDING {}
        };

        D3D12_VIDEO_PROCESS_OUTPUT_STREAM_ARGUMENTS outputArgs{
            {
                mD3D12RGBATexture,
                0, // Output subresource index
            },
            { 0 ,0, (LONG)mWidth, (LONG)mHeight },
        };

        D3D12_RESOURCE_BARRIER preProcessBarriers[] = {
            { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_TRANSITION_BARRIER { mD3D12VideoTexture[subResourceIndex & 1], 0, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ } },
            { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_TRANSITION_BARRIER { mD3D12RGBATexture, 0, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE } },
        };

        D3D12_RESOURCE_BARRIER postProcessBarriers[] = {
            { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_TRANSITION_BARRIER { mD3D12VideoTexture[subResourceIndex & 1], 0, D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ, D3D12_RESOURCE_STATE_COMMON } },
            { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_TRANSITION_BARRIER { mD3D12RGBATexture, 0, D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE, D3D12_RESOURCE_STATE_COMMON } },
        };

        mFrameIndex = (mFrameIndex + 1) & 1;

        // Wait on any pending work if needed since we only have one allocator
        if (gD3D12Fence->GetCompletedValue() < mCommandAllocatorFenceValue[mFrameIndex])
        {
            HANDLE fenceEvent = CreateEvent(nullptr, false, false, nullptr);
            gD3D12Fence->SetEventOnCompletion(mCommandAllocatorFenceValue[mFrameIndex], fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
            CloseHandle(fenceEvent);
        }

        gRenderContext->InsertWait(gD3D12VideoCommandQueue);

        gD3D12VideoCommandQueue->Wait(gD3D12VideoTextureFence, mVideoTextureFenceValue);

        mD3D12VideoCommandAllocator[mFrameIndex]->Reset();
        mD3D12VideoCommandList->Reset(mD3D12VideoCommandAllocator[mFrameIndex]);

        mD3D12VideoCommandList->ResourceBarrier(NS_COUNTOF(preProcessBarriers), preProcessBarriers);
        mD3D12VideoCommandList->ProcessFrames(gD3D12VideoProcessor, &outputArgs, 1, &inputArgs);
        mD3D12VideoCommandList->ResourceBarrier(NS_COUNTOF(postProcessBarriers), postProcessBarriers);

        mD3D12VideoCommandList->Close();
        gD3D12VideoCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&mD3D12VideoCommandList);

        // Signal the fence when its complete
        mCommandAllocatorFenceValue[mFrameIndex] = ++gD3D12FenceValue;
        gD3D12VideoCommandQueue->Signal(gD3D12Fence, mCommandAllocatorFenceValue[mFrameIndex]);

        // Ask the graphics queue to wait till the VP operation is complete before copying to the output
        gRenderContext->mQueue->Wait(gD3D12Fence, mCommandAllocatorFenceValue[mFrameIndex]);
    }

    return mTexture != nullptr ? mTexture : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void D3D12MediaPlayer::TransitionVideoTexture(ID3D12GraphicsCommandList* graphicsCommandList)
{
    struct D3D12Texture : public Texture
    {
        ID3D12Resource* obj;
        D3D12_RESOURCE_STATES state;
    };
    D3D12Texture* d3d12Texture = (D3D12Texture*)mTexture.GetPtr();
    if (d3d12Texture->state != D3D12_RESOURCE_STATE_COMMON)
    {
        D3D12_RESOURCE_BARRIER barrier{ D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, D3D12_RESOURCE_TRANSITION_BARRIER { mD3D12RGBATexture, 0, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON } };
        graphicsCommandList->ResourceBarrier(1, &barrier);

        d3d12Texture->state = D3D12_RESOURCE_STATE_COMMON;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int64_t D3D12MediaPlayer::GetCurrentTimestamp() const
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
