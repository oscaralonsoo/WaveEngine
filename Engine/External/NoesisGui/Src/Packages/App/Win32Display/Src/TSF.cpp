////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


//#undef NS_LOG_TRACE
//#define NS_LOG_TRACE(...) NS_LOG_(NS_LOG_LEVEL_TRACE, __VA_ARGS__)


#include "TSF.h"

#include <NsGui/TextBox.h>
#include <NsGui/PasswordBox.h>
#include <NsGui/CompositionUnderline.h>
#include <NsGui/ScrollViewer.h>
#include <NsGui/Decorator.h>
#include <NsGui/Keyboard.h>
#include <NsGui/IntegrationAPI.h>
#include <NsCore/UTF8.h>
#include <NsCore/Log.h>
#include <NsCore/Vector.h>
#include <NsCore/Delegate.h>
#include <NsDrawing/Rect.h>

#include <msctf.h>
#include <ctffunc.h>


#define VIEW_COOKIE 0

#define V(x) \
    NS_MACRO_BEGIN \
        HRESULT hr = (x); \
        NS_ASSERT(SUCCEEDED(hr)); \
    NS_MACRO_END


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
/// Helper class to manage COM pointers lifetime
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
class ComPtr
{
public:
    ComPtr(): mPtr(0) { }
    ComPtr(T* ptr): mPtr(ptr) { if (mPtr != 0) { mPtr->AddRef(); } }
    ~ComPtr() { if (mPtr != 0) { mPtr->Release(); } }

    operator T*() const { return mPtr; }
    T* operator ->() const { NS_ASSERT(mPtr != 0); return mPtr; }
    T*& Ref() { return mPtr; }

private:
    T* mPtr;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
static void CopyUTF16(Vector<char, 256>& dest, const WCHAR* text, ULONG count)
{
    Vector<WCHAR, 256> source;
    source.Resize(count + 1);
    wcsncpy_s(source.Data(), source.Size(), text, count);
    source[count] = L'\0';

    uint32_t numChars = UTF8::UTF16To8((const uint16_t*)source.Data(), dest.Data(), 256);
    if (numChars > 256)
    {
        dest.Reserve(numChars);
        UTF8::UTF16To8((const uint16_t*)source.Data(), dest.Data(), numChars);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Implementation of Text Framework Services text store ACP interface
////////////////////////////////////////////////////////////////////////////////////////////////////
class TsfTextStore: public ITextStoreACP, public ITfContextOwnerCompositionSink,
    public ITfTextEditSink
{
public:
    TsfTextStore(): mRefs(1), mSink(0), mCategoryMgr(0), mDisplayAttributeMgr(0), mWindow(0),
        mLockType(0), mTextLength(0), mTextBox(0), mEditContext(0), mEditCookie(TF_INVALID_COOKIE),
        mReconversion(0), mReconversionRange(0), mReconversionNeeded(false)
    {
        V(CoCreateInstance(CLSID_TF_CategoryMgr, 0, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr,
            (void**)&mCategoryMgr));

        V(CoCreateInstance(CLSID_TF_DisplayAttributeMgr, 0, CLSCTX_INPROC_SERVER,
            IID_ITfDisplayAttributeMgr, (void**)&mDisplayAttributeMgr));
    }

    ~TsfTextStore()
    {
        if (mEditContext != 0)
        {
            mEditContext->Release();
            mEditContext = 0;
            mEditCookie = TF_INVALID_COOKIE;
        }

        if (mReconversionRange != 0)
        {
            mReconversionRange->Release();
            mReconversionRange = 0;
        }

        if (mReconversion != 0)
        {
            mReconversion->Release();
            mReconversion = 0;
        }

        if (mDisplayAttributeMgr != 0)
        {
            mDisplayAttributeMgr->Release();
            mDisplayAttributeMgr = 0;
        }

        if (mCategoryMgr != 0)
        {
            mCategoryMgr->Release();
            mCategoryMgr = 0;
        }
    }

    void AssociateControl(HWND hWnd, TextBox* control)
    {
        if (mTextBox != 0)
        {
            mTextBox->RemoveHandler(ScrollViewer::ScrollChangedEvent, MakeDelegate(this, &TsfTextStore::OnScrollChanged));
            mTextBox->SelectionChanged() -= MakeDelegate(this, &TsfTextStore::OnControlSelectionChanged);
            mTextBox->TextChanged() -= MakeDelegate(this, &TsfTextStore::OnControlTextChanged);

            mTextLength = 0;
        }

        mWindow = hWnd;
        mTextBox = control;

        if (mTextBox != 0)
        {
            mTextLength = UTF8::Length(mTextBox->GetText());

            mTextBox->TextChanged() += MakeDelegate(this, &TsfTextStore::OnControlTextChanged);
            mTextBox->SelectionChanged() += MakeDelegate(this, &TsfTextStore::OnControlSelectionChanged);
            mTextBox->AddHandler(ScrollViewer::ScrollChangedEvent, MakeDelegate(this, &TsfTextStore::OnScrollChanged));
        }
    }

    void SetEditContext(ITfContext* editContext, TfEditCookie editCookie,
        ITfFnReconversion* reconversion)
    {
        mEditContext = editContext;
        mEditContext->AddRef();

        mEditCookie = editCookie;

        mReconversion = reconversion;
        mReconversion->AddRef();
    }

    bool IsEnabled() const
    {
        return mWindow != 0;
    }

    void OnTextChanged(uint32_t start, uint32_t removed, uint32_t added)
    {
        if (mSink != 0 && !HasReadLock() && !HasWriteLock())
        {
            NS_LOG_TRACE("~> OnTextChanged(start=%u, removed=%u, added=%u", start, removed, added);

            TS_TEXTCHANGE change = { LONG(start), LONG(start + removed), LONG(start + added) };
            mSink->OnTextChange(0, &change);
        }
    }

    void OnSelectionChanged()
    {
        if (mSink != 0 && !HasReadLock() && !HasWriteLock())
        {
            NS_LOG_TRACE("~> OnSelectionChanged()");

            mSink->OnSelectionChange();
        }
    }

    void OnLayoutChanged()
    {
        if (mSink != 0 && !HasReadLock() && !HasWriteLock())
        {
            NS_LOG_TRACE("~> OnLayoutChanged()");

            mSink->OnLayoutChange(TS_LC_CHANGE, VIEW_COOKIE);
        }
    }


    // From IUnknown
    //@{
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override
    {
        *ppvObj = NULL;

        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITextStoreACP))
        {
            *ppvObj = static_cast<ITextStoreACP*>(this);
        }
        else if (IsEqualIID(riid, IID_ITfContextOwnerCompositionSink))
        {
            *ppvObj = static_cast<ITfContextOwnerCompositionSink*>(this);
        }
        else if (IsEqualIID(riid, IID_ITfTextEditSink))
        {
            *ppvObj = static_cast<ITfTextEditSink*>(this);
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

    // From ITextStoreACP
    //@{
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown* punk, DWORD) override
    {
        if (!IsEqualGUID(riid, IID_ITextStoreACPSink))
        {
            return TS_E_NOOBJECT;
        }

        if (FAILED(punk->QueryInterface<ITextStoreACPSink>(&mSink)))
        {
            return E_NOINTERFACE;
        }

        return S_OK;
    }

    STDMETHODIMP UnadviseSink(IUnknown*) override
    {
        // we're dealing with TSF. We don't have to check punk is same instance of mSink.
        if (mSink != 0)
        {
            mSink->Release();
            mSink = NULL;
        }

        return S_OK;
    }

    STDMETHODIMP RequestLock(DWORD dwLockFlags, HRESULT* phrSession) override
    {
        if (mSink == 0 || !IsEnabled())
        {
            return E_FAIL;
        }

        if (phrSession == 0)
        {
            return E_INVALIDARG;
        }

        NS_LOG_TRACE("RequestLock(lockFlags=%ld)", dwLockFlags);

        if (mLockType != 0)
        {
            if ((dwLockFlags & TS_LF_SYNC) != 0)
            {
                NS_LOG_TRACE("--> Error: Already locked");

                // Can't lock synchronously
                *phrSession = TS_E_SYNCHRONOUS;
                return S_OK;
            }
            else
            {
                NS_LOG_TRACE("--> Queue lock");

                // Queue an asynchronous lock
                mLockQueue.PushBack(dwLockFlags & TS_LF_READWRITE);
                *phrSession = TS_S_ASYNC;
                return S_OK;
            }
        }

        // Lock
        NS_LOG_TRACE("--> Lock Begin");
        mLockType = dwLockFlags & TS_LF_READWRITE;
        *phrSession = mSink->OnLockGranted(mLockType);
        mLockType = 0;
        NS_LOG_TRACE("--> Lock End");

        // Handle pending lock requests
        while (!mLockQueue.Empty())
        {
            NS_LOG_TRACE("RequestLock(lockFlags=%ld) [queued]", mLockQueue.Front());

            NS_LOG_TRACE("--> Lock Begin");
            mLockType = mLockQueue.Front();
            mLockQueue.Erase(mLockQueue.Begin());
            mSink->OnLockGranted(mLockType);
            mLockType = 0;
            NS_LOG_TRACE("--> Lock End");
        }

        return S_OK;
    }

    STDMETHODIMP GetStatus(TS_STATUS* pdcs) override
    {
        if (pdcs == 0)
        {
            return E_INVALIDARG;
        }

        // We use transitory contexts and we don't support hidden text
        pdcs->dwDynamicFlags = 0;
        pdcs->dwStaticFlags = TS_SS_TRANSITORY | TS_SS_NOHIDDENTEXT;

        return S_OK;
    }

    STDMETHODIMP QueryInsert(LONG acpInsertStart, LONG acpInsertEnd, ULONG cch,
        LONG* pacpResultStart, LONG* pacpResultEnd) override
    {
        if (pacpResultStart == 0 || pacpResultEnd == 0)
        {
            return E_INVALIDARG;
        }

        NS_LOG_TRACE("QueryInsert(acpStart=%ld, acpEnd=%ld, cch=%lu)", acpInsertStart,
            acpInsertEnd, cch);

        *pacpResultStart = acpInsertStart;
        *pacpResultEnd = acpInsertStart + cch;

        return S_OK;
    }

    STDMETHODIMP GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP* pSelection,
        ULONG* pcFetched) override
    {
        if (ulCount == 0 || pcFetched == 0)
        {
            return E_INVALIDARG;
        }
        if (!HasReadLock())
        {
            return TS_E_NOLOCK;
        }

        NS_LOG_TRACE("GetSelection(ulIndex=%lu, ulCount=%lu)", ulIndex, ulCount);

        *pcFetched = 0;
        if (ulCount > 0 && (ulIndex == 0 || ulIndex == TS_DEFAULT_SELECTION))
        {
            NS_ASSERT(mTextBox != 0);
            pSelection[0].acpStart = mTextBox->GetSelectionStart();
            pSelection[0].acpEnd = mTextBox->GetSelectionLength() + pSelection[0].acpStart;
            pSelection[0].style.ase = TS_AE_END;
            pSelection[0].style.fInterimChar = FALSE;
            *pcFetched = 1;

            NS_LOG_TRACE("--> acpStart=%ld, acpEnd=%ld", pSelection[0].acpStart,
                pSelection[0].acpEnd);
        }

        return S_OK;
    }

    STDMETHODIMP SetSelection(ULONG ulCount, const TS_SELECTION_ACP* pSelection) override
    {
        if (!HasWriteLock())
        {
            return TF_E_NOLOCK;
        }

        NS_LOG_TRACE("SetSelection(ulCount=%lu, Sel.acpStart=%ld, Sel.acpEnd=%ld)", ulCount,
            pSelection[0].acpStart, pSelection[0].acpEnd);

        if (ulCount > 0)
        {
            NS_ASSERT(mTextBox != 0);
            mTextBox->SetSelectionStart(pSelection[0].acpStart);
            mTextBox->SetSelectionLength(pSelection[0].acpEnd - pSelection[0].acpStart);
        }

        return S_OK;
    }

    STDMETHODIMP GetText(LONG acpStart, LONG acpEnd, __out_ecount(cchPlainReq) WCHAR* pchPlain,
        ULONG cchPlainReq, ULONG* pcchPlainOut, TS_RUNINFO* prgRunInfo, ULONG ulRunInfoReq,
        ULONG* pulRunInfoOut, LONG* pacpNext) override
    {
        if (pcchPlainOut == 0 || pulRunInfoOut == 0 || pacpNext == 0)
        {
            return E_INVALIDARG;
        }
        if (pchPlain == 0 && cchPlainReq != 0)
        {
            return E_INVALIDARG;
        }
        if (prgRunInfo == 0 && ulRunInfoReq != 0)
        {
            return E_INVALIDARG;
        }
        if (acpStart < 0 || acpEnd < -1)
        {
            return E_INVALIDARG;
        }
        if (!HasReadLock())
        {
            return TF_E_NOLOCK;
        }

        *((uint16_t*)pchPlain) = 0;

        if (cchPlainReq == 0 && ulRunInfoReq == 0)
        {
            return S_OK;
        }

        NS_LOG_TRACE("GetText(acpStart=%ld, acpEnd=%ld, cchPlainReq=%lu)",
            acpStart, acpEnd, cchPlainReq);

        NS_ASSERT(mTextBox != 0);
        const char* text = mTextBox->GetText();

        if (acpEnd == -1)
        {
            acpEnd = mTextLength;
        }

        acpEnd = Min(acpEnd, acpStart + (int)cchPlainReq);

        if (acpStart < acpEnd)
        {
            UTF8::Advance(text, acpStart);
            UTF8::UTF8To16(text, (uint16_t*)pchPlain, acpEnd - acpStart + 1);
        }

        NS_LOG_TRACE("--> text=\"%s\", cch=%ld", text, acpEnd - acpStart);

        *pcchPlainOut = acpEnd - acpStart;

        if (ulRunInfoReq != 0)
        {
            prgRunInfo[0].uCount = acpEnd - acpStart;
            prgRunInfo[0].type = TS_RT_PLAIN;
            *pulRunInfoOut = 1;
        }

        *pacpNext = acpEnd;

        return S_OK;
    }

    STDMETHODIMP SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd,
        __in_ecount(cch) const WCHAR* pchText, ULONG cch, TS_TEXTCHANGE* pChange) override
    {
        if (acpStart < 0 || acpEnd < 0)
        {
            return E_INVALIDARG;
        }
        if (!HasWriteLock())
        {
            return TF_E_NOLOCK;
        }

        if (acpStart > (LONG)mTextLength)
        {
            return E_INVALIDARG;
        }

        NS_LOG_TRACE("SetText(flags=%ld, acpStart=%ld, acpEnd=%ld)", dwFlags, acpStart, acpEnd);

        // A reconversion can be initiated when user presses the Space key and some text is,
        // selected, in that case the conversion candidate list is shown and we are finished here
        if (TryReconversion(pchText, cch))
        {
            return S_OK;
        }

        NS_ASSERT(mTextBox != 0);
        mTextBox->SetSelectionStart(acpStart);
        mTextBox->SetSelectionLength(acpEnd - acpStart);

        Vector<char, 256> buffer;
        CopyUTF16(buffer, pchText, cch);

        NS_LOG_TRACE("--> text=\"%s\", cch=%lu", buffer.Data(), cch);

        mTextBox->SetSelectedText(buffer.Data());

        if (pChange != 0)
        {
            pChange->acpStart = acpStart;
            pChange->acpOldEnd = acpEnd;
            pChange->acpNewEnd = acpStart + cch;
        }

        return S_OK;
    }

    STDMETHODIMP GetFormattedText(LONG, LONG, IDataObject**) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP GetEmbedded(LONG, REFGUID, REFIID, IUnknown**) override
    {
        // We don't support any embedded objects.
        return E_NOTIMPL;
    }

    STDMETHODIMP InsertEmbedded(DWORD, LONG, LONG, IDataObject*, TS_TEXTCHANGE*) override
    {
        // We don't support any embedded objects.
        return E_NOTIMPL;
    }

    STDMETHODIMP RequestSupportedAttrs(DWORD, ULONG, const TS_ATTRID* paFilterAttrs) override
    {
        if (paFilterAttrs == 0)
        {
            return E_INVALIDARG;
        }

        // We don't support attributes
        return E_FAIL;
    }

    STDMETHODIMP RequestAttrsAtPosition(LONG, ULONG, const TS_ATTRID*, DWORD) override
    {
        // We don't support any document attributes. This method just returns S_OK, and subsequently
        // called RetrieveRequestedAttrs() returns 0 as the number of supported attributes
        return S_OK;
    }

    STDMETHODIMP RequestAttrsTransitioningAtPosition(LONG, ULONG, const TS_ATTRID*, DWORD) override
    {
        // We don't support any document attributes. This method just returns S_OK, and subsequently
        // called RetrieveRequestedAttrs() returns 0 as the number of supported attributes
        return S_OK;
    }

    STDMETHODIMP FindNextAttrTransition(LONG, LONG, ULONG, const TS_ATTRID*, DWORD,
        LONG* pacpNext, BOOL* pfFound, LONG* plFoundOffset) override
    {
        if (pacpNext == 0 || pfFound == 0 || plFoundOffset == 0)
        {
            return E_INVALIDARG;
        }

        // We don't support any attributes, so we always return "not found"
        *pacpNext = 0;
        *pfFound = FALSE;
        *plFoundOffset = 0;

        return S_OK;
    }

    STDMETHODIMP RetrieveRequestedAttrs(ULONG, TS_ATTRVAL* paAttrVals, ULONG* pcFetched) override
    {
        if (paAttrVals == 0 || pcFetched == 0)
        {
            return E_INVALIDARG;
        }

        *pcFetched = 0;
        return S_OK;
    }

    STDMETHODIMP GetEndACP(LONG* pacp) override
    {
        if (pacp == 0)
        {
            return E_INVALIDARG;
        }
        if (!HasReadLock())
        {
            return TS_E_NOLOCK;
        }

        *pacp = (LONG)mTextLength;

        return S_OK;
    }

    STDMETHODIMP GetActiveView(TsViewCookie* pvcView) override
    {
        // We support only one view
        *pvcView = VIEW_COOKIE;
        return S_OK;
    }

    STDMETHODIMP GetACPFromPoint(TsViewCookie, const POINT*, DWORD, LONG*) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT* prc,
        BOOL* pfClipped) override
    {
        if (prc == 0 || pfClipped == 0 || vcView != VIEW_COOKIE)
        {
            return E_INVALIDARG;
        }
        if (acpStart < 0 || acpEnd < 0)
        {
            return E_INVALIDARG;
        }
        if (!HasReadLock())
        {
            return TF_E_NOLOCK;
        }

        NS_LOG_TRACE("GetTextExt(vcView=%ld, acpStart=%ld, acpEnd=%ld)", vcView, acpStart, acpEnd);

        // Make sure control is measured and arranged correctly
        NS_ASSERT(mTextBox != 0);
        mTextBox->UpdateLayout();

        // UpdateLayout can release the focus from the TextBox, in that case we just return
        if (mTextBox == 0)
        {
            return E_INVALIDARG;
        }

        // Get bounding box of the selected text
        Rect bounds = mTextBox->GetRangeBounds(acpStart, acpEnd);

        // Add some space for the underlines
        float fontSize = mTextBox->GetFontSize();
        bounds.height += Max(1.0f, fontSize * 0.1f);

        // Convert bounding box vertices to client coordinates
        Visual* textView = mTextBox->GetTextView();
        Point topLeft = textView->PointToScreen(bounds.GetTopLeft());
        Point bottomRight = textView->PointToScreen(bounds.GetBottomRight());

        prc->left = (LONG)Round(topLeft.x);
        prc->right = (LONG)Round(bottomRight.x);
        prc->top = (LONG)Round(topLeft.y);
        prc->bottom = (LONG)Round(bottomRight.y);

        ClientToScreen(mWindow, (POINT*)&prc->left);
        ClientToScreen(mWindow, (POINT*)&prc->right);

        NS_LOG_TRACE("--> (%ld, %ld), %ldx%ld",
            prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top);

        *pfClipped = FALSE;

        return S_OK;
    }

    STDMETHODIMP GetScreenExt(TsViewCookie vcView, RECT* prc) override
    {
        if (prc == 0 || vcView != VIEW_COOKIE)
        {
            return E_INVALIDARG;
        }

        NS_LOG_TRACE("GetScreenExt(vcView=%ld)", vcView);

        GetClientRect(mWindow, prc);

        ClientToScreen(mWindow, (POINT*)&prc->left);
        ClientToScreen(mWindow, (POINT*)&prc->right);

        return S_OK;
    }

    STDMETHODIMP GetWnd(TsViewCookie vcView, HWND* phWnd) override
    {
        if (phWnd == 0 || vcView != VIEW_COOKIE)
        {
            return E_INVALIDARG;
        }

        *phWnd = mWindow;

        return S_OK;
    }

    STDMETHODIMP QueryInsertEmbedded(const GUID*, const FORMATETC*, BOOL*) override
    {
        // We don't support any embedded objects.
        return E_NOTIMPL;
    }

    STDMETHODIMP InsertTextAtSelection(DWORD dwFlags, __in_ecount(cch) const WCHAR* pchText,
        ULONG cch, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange) override
    {
        NS_LOG_TRACE("InsertTextAtSelection(flags=%ld)", dwFlags);

        NS_ASSERT(mTextBox != 0);
        LONG acpStart = (LONG)mTextBox->GetSelectionStart();
        LONG acpEnd = (LONG)mTextBox->GetSelectionLength() + acpStart;

        if ((dwFlags & TS_IAS_QUERYONLY) != 0)
        {
            if (!HasReadLock())
            {
                return TF_E_NOLOCK;
            }

            if (pacpStart != 0)
            {
                *pacpStart = acpStart;
            }
            if (pacpEnd != 0)
            {
                *pacpEnd = acpEnd;
            }

            NS_LOG_TRACE("--> QueryOnly: acpStart=%ld, acpEnd=%ld", acpStart, acpEnd);

            return S_OK;
        }

        if (pchText == 0)
        {
            return E_INVALIDARG;
        }
        if (!HasWriteLock())
        {
            return TS_E_NOLOCK;
        }

        if (pchText != 0 && cch > 0)
        {
            Vector<char, 256> buffer;
            CopyUTF16(buffer, pchText, cch);

            NS_ASSERT(mTextBox != 0);
            mTextBox->SetSelectedText(buffer.Data());

            NS_LOG_TRACE("--> text=\"%s\", cch=%lu", buffer.Data(), cch);
        }

        if (pacpStart != 0)
        {
            *pacpStart = acpStart;
        }
        if (pacpEnd != 0)
        {
            *pacpEnd = acpStart + cch;
        }
        if (pChange != 0)
        {
            pChange->acpStart = acpStart;
            pChange->acpOldEnd = acpEnd;
            pChange->acpNewEnd = acpStart + cch;
        }

        NS_ASSERT(mTextBox != 0);
        mTextBox->SetSelectionStart(acpStart);
        mTextBox->SetSelectionLength(cch);

        NS_LOG_TRACE("--> acpStart=%ld, acpEnd=%ld", acpStart, acpStart + cch);

        return S_OK;
    }

    STDMETHODIMP InsertEmbeddedAtSelection(DWORD, IDataObject*, LONG*, LONG*,
        TS_TEXTCHANGE*) override
    {
        // We don't support any embedded objects.
        return E_NOTIMPL;
    }
    //@}

    // From ITfContextOwnerCompositionSink
    //@{
    STDMETHODIMP OnStartComposition(ITfCompositionView* pComposition, BOOL* pfOk) override
    {
        NS_LOG_TRACE("OnStartComposition(composition=%p)", pComposition);

        if (pfOk != 0)
        {
            *pfOk = TRUE;
        }

        return S_OK;
    }

    STDMETHODIMP OnUpdateComposition(ITfCompositionView * pComposition, ITfRange* pRange) override
    {
        NS_LOG_TRACE("OnUpdateComposition(composition=%p, range=%p)", pComposition, pRange);
        return S_OK;
    }

    STDMETHODIMP OnEndComposition(ITfCompositionView* pComposition) override
    {
        NS_LOG_TRACE("OnEndComposition(composition=%p)", pComposition);
        return S_OK;
    }
    //@}

    // From ITfTextEditSink
    //@{
    STDMETHODIMP OnEndEdit(ITfContext* context, TfEditCookie ecReadOnly,
        ITfEditRecord* pEditRecord) override
    {
        if (context == 0 || pEditRecord == 0)
        {
            return E_INVALIDARG;
        }

        if (!HasWriteLock())
        {
            // nothing modified
            return S_OK;
        }

        NS_LOG_TRACE("OnEndEdit(context=%p, editCookie=%ld, editRecor=%p)", context, ecReadOnly,
            pEditRecord);

        const GUID* guids[2] = { &GUID_PROP_COMPOSING, &GUID_PROP_ATTRIBUTE };
        ComPtr<ITfReadOnlyProperty> trackProperty;
        V(context->TrackProperties(guids, 2, NULL, 0, &trackProperty.Ref()));

        NS_ASSERT(mTextBox != 0);
        mTextBox->ClearCompositionUnderlines();

        ComPtr<ITfRange> startEndRange;
        ComPtr<ITfRange> endRange;
        V(context->GetStart(ecReadOnly, &startEndRange.Ref()));
        V(context->GetEnd(ecReadOnly, &endRange.Ref()));
        V(startEndRange->ShiftEndToRange(ecReadOnly, endRange, TF_ANCHOR_END));

        ComPtr<IEnumTfRanges> ranges;
        V(trackProperty->EnumRanges(ecReadOnly, &ranges.Ref(), startEndRange));

        for (;;)
        {
            ULONG fetched;
            ComPtr<ITfRange> range;
            if (ranges->Next(1, &range.Ref(), &fetched) != S_OK)
            {
                break;
            }

            VARIANT value;
            ComPtr<IEnumTfPropertyValue> enumPropertyValue;
            V(trackProperty->GetValue(ecReadOnly, range, &value));
            NS_ASSERT(value.punkVal != 0);
            V(value.punkVal->QueryInterface<IEnumTfPropertyValue>(&enumPropertyValue.Ref()));

            bool isComposing = false;
            bool hasDisplayAttribute = false;
            TF_PROPERTYVAL propertyValue = TF_PROPERTYVAL();
            TF_DISPLAYATTRIBUTE displayAttribute = TF_DISPLAYATTRIBUTE();
            while (enumPropertyValue->Next(1, &propertyValue, &fetched) == S_OK)
            {
                if (IsEqualGUID(propertyValue.guidId, GUID_PROP_COMPOSING))
                {
                    isComposing = propertyValue.varValue.lVal == TRUE;
                }
                else if (IsEqualGUID(propertyValue.guidId, GUID_PROP_ATTRIBUTE))
                {
                    TfGuidAtom atom = static_cast<TfGuidAtom>(propertyValue.varValue.lVal);
                    hasDisplayAttribute = GetDisplayAttribute(atom, &displayAttribute);
                }
                VariantClear(&propertyValue.varValue);
            }

            ComPtr<ITfRangeACP> acpRange;
            V(range->QueryInterface<ITfRangeACP>(&acpRange.Ref()));

            LONG start, length;
            V(acpRange->GetExtent(&start, &length));
            if (isComposing)
            {
                CompositionLineStyle style = (CompositionLineStyle)displayAttribute.lsStyle;
                CompositionUnderline underline = { uint32_t(start), uint32_t(start + length), style,
                    displayAttribute.fBoldLine == TRUE ? true : false };
                mTextBox->AddCompositionUnderline(underline);

                NS_LOG_TRACE("--> Underline: %u-%u, %d, %s", underline.start, underline.end,
                    (int)style, underline.bold ? "Bold" : "Normal");
            }

            VariantClear(&value);
        }

        if (mReconversionNeeded)
        {
            mReconversionNeeded = false;
            DoReconversion();
        }

        return S_OK;
    }
    //@}

private:
    bool HasReadLock() const
    {
        return (mLockType & TS_LF_READ) == TS_LF_READ;
    }

    bool HasWriteLock() const
    {
        return (mLockType & TS_LF_READWRITE) == TS_LF_READWRITE;
    }

    bool IsSpaceChar(WCHAR ch)
    {
        const WCHAR halfWidthSpace = 0x0020;
        const WCHAR fullWidthSpace = 0x3000;
        return ch == halfWidthSpace || ch == fullWidthSpace;
    }

    bool TryReconversion(const WCHAR* text, ULONG len)
    {
        NS_ASSERT(mTextBox != 0);
        if (mTextBox->GetSelectionLength() > 0)
        {
            if ((len == 0 || len == 1) && IsSpaceChar(*text))
            {
                if (TryReconversion())
                {
                    mReconversionNeeded = true;
                    return true;
                }
            }
        }

        return false;
    }

    bool TryReconversion()
    {
        NS_ASSERT(mEditContext != 0);
        NS_ASSERT(mReconversion != 0);

        NS_LOG_TRACE("TryReconversion()");

        ULONG fetched = 0;
        TF_SELECTION selection = TF_SELECTION();
        V(mEditContext->GetSelection(mEditCookie, TF_DEFAULT_SELECTION, 1, &selection, &fetched));

        NS_ASSERT(fetched > 0);

        BOOL convertable = FALSE;
        ComPtr<ITfRange> range;
        V(mReconversion->QueryRange(selection.range, &range.Ref(), &convertable));

        if (convertable != FALSE && range != 0)
        {
            mReconversionRange = (ITfRange*)range;
            mReconversionRange->AddRef();

            return true;
        }

        return false;
    }

    void DoReconversion()
    {
        NS_ASSERT(mReconversion != 0);
        NS_ASSERT(mReconversionRange != 0);

        NS_LOG_TRACE("DoReconversion()");

        V(mReconversion->Reconvert(mReconversionRange));

        mReconversionRange->Release();
        mReconversionRange = 0;
    }

    bool GetDisplayAttribute(TfGuidAtom atom, TF_DISPLAYATTRIBUTE* attribute) const
    {
        GUID guid;
        if (FAILED(mCategoryMgr->GetGUID(atom, &guid)))
        {
            return false;
        }

        ComPtr<ITfDisplayAttributeInfo> info;
        if (FAILED(mDisplayAttributeMgr->GetDisplayAttributeInfo(guid, &info.Ref(), NULL)))
        {
            return false;
        }

        return SUCCEEDED(info->GetAttributeInfo(attribute));
    }

    void OnControlTextChanged(BaseComponent*, const RoutedEventArgs&)
    {
        NS_ASSERT(mTextBox != 0);
        uint32_t newLength = UTF8::Length(mTextBox->GetText());
        uint32_t removed = mTextLength > newLength ? mTextLength - newLength : 0;
        uint32_t added = mTextLength < newLength ? newLength - mTextLength : 0;
        mTextLength = newLength;

        OnTextChanged(mTextBox->GetSelectionStart(), removed, added);
    }

    void OnControlSelectionChanged(BaseComponent*, const RoutedEventArgs&)
    {
        OnSelectionChanged();
    }

    void OnScrollChanged(BaseComponent*, const RoutedEventArgs&)
    {
        OnLayoutChanged();
    }

private:
    ULONG mRefs;
    ITextStoreACPSink* mSink;
    ITfCategoryMgr* mCategoryMgr;
    ITfDisplayAttributeMgr* mDisplayAttributeMgr;
    HWND mWindow;
    DWORD mLockType;
    uint32_t mTextLength;

    TextBox* mTextBox;
    ITfContext* mEditContext;
    TfEditCookie mEditCookie;
    ITfFnReconversion* mReconversion;
    ITfRange* mReconversionRange;
    bool mReconversionNeeded;

    Vector<DWORD> mLockQueue;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
int gInitCount = 0;
ITfThreadMgr* gThreadMgr = 0;
TfClientId gClientId = TF_CLIENTID_NULL;
ITfDocumentMgr* gTextBoxDocumentMgr = 0;
TsfTextStore* gTextBoxStore = 0;
DWORD gTextBoxEditCookie = TF_INVALID_COOKIE;
HWND gCurrentWindow = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
static void SetCompartmentValue(ITfCompartmentMgr* compartmentMgr, const GUID& key, DWORD value)
{
    ComPtr<ITfCompartment> compartment;
    V(compartmentMgr->GetCompartment(key, &compartment.Ref()));

    VARIANT variant;
    ::VariantInit(&variant);
    variant.vt = VT_I4;
    variant.lVal = value;
    V(compartment->SetValue(gClientId, &variant));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void CreateTextBoxDocument()
{
    V(gThreadMgr->CreateDocumentMgr(&gTextBoxDocumentMgr));

    gTextBoxStore = new TsfTextStore();

    TfEditCookie inputCookie;
    ComPtr<ITfContext> inputContext;
    V(gTextBoxDocumentMgr->CreateContext(gClientId, 0, (ITextStoreACP*)gTextBoxStore,
        &inputContext.Ref(), &inputCookie));

    ComPtr<ITfFunctionProvider> functionProvider;
    V(gThreadMgr->GetFunctionProvider(GUID_SYSTEM_FUNCTIONPROVIDER,
        &functionProvider.Ref()));

    ComPtr<ITfFnReconversion> reconversion;
    V(functionProvider->GetFunction(GUID_NULL, IID_ITfFnReconversion,
        (IUnknown**)&reconversion.Ref()));

    gTextBoxStore->SetEditContext(inputContext, inputCookie, reconversion);

    V(gTextBoxDocumentMgr->Push(inputContext));

    ComPtr<ITfSource> source;
    V(inputContext->QueryInterface<ITfSource>(&source.Ref()));

    V(source->AdviseSink(IID_ITfTextEditSink, (ITfTextEditSink*)gTextBoxStore,
        &gTextBoxEditCookie));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Japanese IME expects the default value of this compartment to be TF_SENTENCEMODE_PHRASEPREDICT
// like IMM32 implementation. This value does not affect other language's IME behaviors
static void ConfigureJapaneseIME()
{
    ComPtr<ITfCompartmentMgr> compartmentMgr;
    V(gThreadMgr->QueryInterface<ITfCompartmentMgr>(&compartmentMgr.Ref()));

    SetCompartmentValue(compartmentMgr, GUID_COMPARTMENT_KEYBOARD_INPUTMODE_SENTENCE,
        TF_SENTENCEMODE_PHRASEPREDICT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void DestroyTextBoxDocument()
{
    if (gTextBoxDocumentMgr != 0)
    {
        ComPtr<ITfContext> inputContext;
        V(gTextBoxDocumentMgr->GetTop(&inputContext.Ref()));

        if (gTextBoxEditCookie != TF_INVALID_COOKIE)
        {
            ComPtr<ITfSource> source;
            V(inputContext->QueryInterface<ITfSource>(&source.Ref()));

            V(source->UnadviseSink(gTextBoxEditCookie));
            gTextBoxEditCookie = TF_INVALID_COOKIE;
        }

        gTextBoxDocumentMgr->Pop(TF_POPF_ALL);
        gTextBoxDocumentMgr->Release();
        gTextBoxDocumentMgr = 0;
    }

    if (gTextBoxStore != 0)
    {
        gTextBoxStore->Release();
        gTextBoxStore = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void CancelComposition()
{
    ComPtr<ITfContext> inputContext;
    V(gTextBoxDocumentMgr->GetBase(&inputContext.Ref()));

    ComPtr<ITfContextOwnerCompositionServices> owner;
    V(inputContext->QueryInterface<ITfContextOwnerCompositionServices>(&owner.Ref()));

    V(owner->TerminateComposition(0));
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void TSF::Init()
{
    if (gInitCount++ == 0)
    {
        V(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));

        V(CoCreateInstance(CLSID_TF_ThreadMgr, 0, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr,
            (void**)&gThreadMgr));

        V(gThreadMgr->Activate(&gClientId));

        CreateTextBoxDocument();

        ConfigureJapaneseIME();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TSF::Shutdown()
{
    if (gInitCount > 0 && --gInitCount == 0)
    {
        gCurrentWindow = 0;

        DestroyTextBoxDocument();

        if (gThreadMgr != 0)
        {
            V(gThreadMgr->Deactivate());
            gThreadMgr->Release();
            gThreadMgr = 0;
            gClientId = TF_CLIENTID_NULL;
        }

        CoUninitialize();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TSF::ActivateWindow(void* hWnd)
{
    NS_LOG_TRACE("ActivateWindow(hWnd=%p)", hWnd);
    NS_ASSERT(gCurrentWindow == 0 || gCurrentWindow == hWnd);
    gCurrentWindow = (HWND)hWnd;

    // NOTE: When hWnd is activated, the associated Noesis View automatically restores keyboard
    // focus to the corresponding control. If it is a TextBox, then OnShowKeyboard will be called.
    // But calls to gThreadMgr->SetFocus() inside WM_ACTIVATE seem to have no effect, so we need to
    // do it later. We enqueue a move message and we will call SetFocus() inside TSF::MoveWindow().
    PostMessage(gCurrentWindow, WM_EXITSIZEMOVE, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TSF::DeactivateWindow(void* hWnd)
{
    NS_LOG_TRACE("DeactivateWindow(hWnd=%p)", hWnd);
    NS_ASSERT(gCurrentWindow == hWnd);
    gCurrentWindow = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TSF::MoveWindow(void* hWnd)
{
    NS_LOG_TRACE("MoveWindow(hWnd=%p)", hWnd);
    if (gCurrentWindow == hWnd && gThreadMgr != 0)
    {
        ITfDocumentMgr* doc;

        NS_ASSERT(gTextBoxStore != 0);
        if (gTextBoxStore->IsEnabled())
        {
            V(gThreadMgr->AssociateFocus(gCurrentWindow, gTextBoxDocumentMgr, &doc));
            gTextBoxStore->OnLayoutChanged();
        }
        else
        {
            V(gThreadMgr->AssociateFocus(gCurrentWindow, NULL, &doc));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TSF::ShowKeyboard(UIElement* focused)
{
    if (gCurrentWindow != 0)
    {
        ITfDocumentMgr* doc;

        TextBox* textBox = DynamicCast<TextBox*>(focused);
        if (textBox != 0)
        {
            gTextBoxStore->AssociateControl(gCurrentWindow, textBox);
            V(gThreadMgr->AssociateFocus(gCurrentWindow, gTextBoxDocumentMgr, &doc));
        }
        else
        {
            V(gThreadMgr->AssociateFocus(gCurrentWindow, NULL, &doc));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TSF::HideKeyboard()
{
    if (gTextBoxStore->IsEnabled())
    {
        CancelComposition();

        gTextBoxStore->AssociateControl(0, 0);
    }

    if (gCurrentWindow != 0)
    {
        ITfDocumentMgr* doc;
        V(gThreadMgr->AssociateFocus(gCurrentWindow, NULL, &doc));
    }
}
