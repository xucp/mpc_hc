/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "RenderersSettings.h"

#include "VMR7AllocatorPresenter.h"
#include "IPinHook.h"

extern bool g_bNoDuration; // Defined in MainFrm.cpp
extern bool g_bExternalSubtitleTime;

using namespace DSObjects;

//
// CVMR7AllocatorPresenter
//

#define MY_USER_ID 0x6ABE51

CVMR7AllocatorPresenter::CVMR7AllocatorPresenter(HWND hWnd, HRESULT& hr)
    : CDX7AllocatorPresenter(hWnd, hr)
    , m_fUseInternalTimer(false)
{
    if (FAILED(hr)) {
        return;
    }

    if (FAILED(hr = m_pSA.CoCreateInstance(CLSID_AllocPresenter))) {
        hr = E_FAIL;
        return;
    }
}

STDMETHODIMP CVMR7AllocatorPresenter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

    return
        QI(IVMRSurfaceAllocator)
        QI(IVMRImagePresenter)
        QI(IVMRWindowlessControl)
        __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CVMR7AllocatorPresenter::CreateDevice()
{
    HRESULT hr = __super::CreateDevice();
    if (FAILED(hr)) {
        return hr;
    }

    if (m_pIVMRSurfAllocNotify) {
        HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        if (FAILED(hr = m_pIVMRSurfAllocNotify->ChangeDDrawDevice(m_pDD, hMonitor))) {
            return hr;    //return false;
        }
    }

    return hr;
}

void CVMR7AllocatorPresenter::DeleteSurfaces()
{
    CAutoLock cAutoLock(this);

    m_pSA->FreeSurface(MY_USER_ID);

    __super::DeleteSurfaces();
}

// ISubPicAllocatorPresenter

STDMETHODIMP CVMR7AllocatorPresenter::CreateRenderer(IUnknown** ppRenderer)
{
    CheckPointer(ppRenderer, E_POINTER);

    *ppRenderer = nullptr;
    CComPtr<IBaseFilter> pBF;

    if (FAILED(pBF.CoCreateInstance(CLSID_VideoMixingRenderer))) {
        return E_FAIL;
    }

    CComQIPtr<IVMRFilterConfig> pConfig = pBF;
    if (!pConfig) {
        return E_FAIL;
    }

    if (FAILED(pConfig->SetRenderingMode(VMRMode_Renderless))) {
        return E_FAIL;
    }

    CComQIPtr<IVMRSurfaceAllocatorNotify> pSAN = pBF;
    if (!pSAN) {
        return E_FAIL;
    }

    if (FAILED(pSAN->AdviseSurfaceAllocator(MY_USER_ID, static_cast<IVMRSurfaceAllocator*>(this)))
            || FAILED(AdviseNotify(pSAN))) {
        return E_FAIL;
    }

    CComPtr<IPin> pPin = GetFirstPin(pBF);
    CComQIPtr<IMemInputPin> pMemInputPin = pPin;
    m_fUseInternalTimer = HookNewSegmentAndReceive((IPinC*)(IPin*)pPin, (IMemInputPinC*)(IMemInputPin*)pMemInputPin);

    *ppRenderer = (IUnknown*)pBF.Detach();

    return S_OK;
}

STDMETHODIMP_(void) CVMR7AllocatorPresenter::SetTime(REFERENCE_TIME rtNow)
{
    __super::SetTime(rtNow);
    //  m_fUseInternalTimer = false;
}

// IVMRSurfaceAllocator

STDMETHODIMP CVMR7AllocatorPresenter::AllocateSurface(DWORD_PTR dwUserID, VMRALLOCATIONINFO* lpAllocInfo, DWORD* lpdwBuffer, LPDIRECTDRAWSURFACE7* lplpSurface)
{
    if (!lpAllocInfo || !lpdwBuffer || !lplpSurface) {
        return E_POINTER;
    }

    if (!m_pIVMRSurfAllocNotify) {
        return E_FAIL;
    }

    HRESULT hr;

    DeleteSurfaces();

    // HACK: yv12 will fail to blt onto the backbuffer anyway, but if we first
    // allocate it and then let our FreeSurface callback call m_pSA->FreeSurface,
    // then that might stall for about 30 seconds because of some unknown buggy code
    // behind <ddraw surface>->Release()

    if (lpAllocInfo->lpHdr->biBitCount < 16) {
        return E_FAIL;
    }

    hr = m_pSA->AllocateSurface(dwUserID, lpAllocInfo, lpdwBuffer, lplpSurface);
    if (FAILED(hr)) {
        return hr;
    }

    CSize VideoSize(abs(lpAllocInfo->lpHdr->biWidth), abs(lpAllocInfo->lpHdr->biHeight));
    CSize AspectRatio(lpAllocInfo->szAspectRatio.cx, lpAllocInfo->szAspectRatio.cy);
    SetVideoSize(VideoSize, AspectRatio);

    if (FAILED(hr = AllocSurfaces())) {
        return hr;
    }

    // test if the colorspace is acceptable
    if (FAILED(hr = m_pVideoSurface->Blt(nullptr, *lplpSurface, nullptr, DDBLT_WAIT, nullptr))) {
        DeleteSurfaces();
        return hr;
    }

    DDBLTFX fx;
    INITDDSTRUCT(fx);
    fx.dwFillColor = 0;
    m_pVideoSurface->Blt(nullptr, nullptr, nullptr, DDBLT_WAIT | DDBLT_COLORFILL, &fx);

    return hr;
}

STDMETHODIMP CVMR7AllocatorPresenter::FreeSurface(DWORD_PTR dwUserID)
{
    DeleteSurfaces();
    return S_OK;
}

STDMETHODIMP CVMR7AllocatorPresenter::PrepareSurface(DWORD_PTR dwUserID, IDirectDrawSurface7* lpSurface, DWORD dwSurfaceFlags)
{
    SetThreadName(DWORD(-1), "CVMR7AllocatorPresenter");

    CheckPointer(lpSurface, E_POINTER);

    // FIXME: sometimes the msmpeg4/divx3/wmv decoder wants to reuse our
    // surface (expects it to point to the same mem every time), and to avoid
    // problems we can't call m_pSA->PrepareSurface (flips? clears?).
    return S_OK;
    /*
        return m_pSA->PrepareSurface(dwUserID, lpSurface, dwSurfaceFlags);
    */
}

STDMETHODIMP CVMR7AllocatorPresenter::AdviseNotify(IVMRSurfaceAllocatorNotify* lpIVMRSurfAllocNotify)
{
    CAutoLock cAutoLock(this);

    m_pIVMRSurfAllocNotify = lpIVMRSurfAllocNotify;
    HRESULT hr;
    HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

    if (FAILED(hr = m_pIVMRSurfAllocNotify->SetDDrawDevice(m_pDD, hMonitor))) {
        return hr;
    }

    return m_pSA->AdviseNotify(lpIVMRSurfAllocNotify);
}

// IVMRImagePresenter

STDMETHODIMP CVMR7AllocatorPresenter::StartPresenting(DWORD_PTR dwUserID)
{
    if (!m_bPendingResetDevice) {
        ASSERT(m_pD3DDev);
    }

    CAutoLock cAutoLock(this);

    m_bIsRendering = true;

    return m_pD3DDev ? S_OK : E_FAIL;
}

STDMETHODIMP CVMR7AllocatorPresenter::StopPresenting(DWORD_PTR dwUserID)
{
    m_bIsRendering = false;

    return S_OK;
}

STDMETHODIMP CVMR7AllocatorPresenter::PresentImage(DWORD_PTR dwUserID, VMRPRESENTATIONINFO* lpPresInfo)
{
    if (!lpPresInfo || !lpPresInfo->lpSurf) {
        return E_POINTER;
    }

    CAutoLock cAutoLock(this);

    if (!m_bPendingResetDevice) {
        m_pVideoSurface->Blt(nullptr, lpPresInfo->lpSurf, nullptr, DDBLT_WAIT, nullptr);
    }

    if (lpPresInfo->rtEnd > lpPresInfo->rtStart) {
        REFERENCE_TIME rtTimePerFrame = lpPresInfo->rtEnd - lpPresInfo->rtStart;
        m_fps = 10000000.0 / rtTimePerFrame;
    } else {
        TRACE(_T("VMR7: Invalid frame timestamps (%s - %s), not setting the FPS. The timestamp from the pin hook will be used anyway (%s).\n"),
              ReftimeToString(lpPresInfo->rtStart), ReftimeToString(lpPresInfo->rtEnd), ReftimeToString(g_tSampleStart));
    }

    if (m_pSubPicQueue) {
        m_pSubPicQueue->SetFPS(m_fps);

        if (m_fUseInternalTimer && !g_bExternalSubtitleTime) {
            __super::SetTime(g_tSegmentStart + g_tSampleStart);
        }
    }

    CSize VideoSize = m_nativeVideoSize;
    int arx = lpPresInfo->szAspectRatio.cx, ary = lpPresInfo->szAspectRatio.cy;
    if (arx > 0 && ary > 0) {
        VideoSize.cx = VideoSize.cy * arx / ary;
    }
    if (VideoSize != GetVideoSize()) {
        SetVideoSize(m_nativeVideoSize, CSize(lpPresInfo->szAspectRatio));
        AfxGetApp()->m_pMainWnd->PostMessage(WM_REARRANGERENDERLESS);
    }

    Paint(true);

    return S_OK;
}

// IVMRWindowlessControl
//
// It is only implemented (partially) for the dvd navigator's
// menu handling, which needs to know a few things about the
// location of our window.

STDMETHODIMP CVMR7AllocatorPresenter::GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight)
{
    CSize VideoSize = GetVideoSize(); // DVD Nav. bug workaround fix

    if (lpWidth) {
        *lpWidth = VideoSize.cx;
    }
    if (lpHeight) {
        *lpHeight = VideoSize.cy;
    }
    if (lpARWidth) {
        *lpARWidth = m_aspectRatio.cx;
    }
    if (lpARHeight) {
        *lpARHeight = m_aspectRatio.cy;
    }
    return S_OK;
}

STDMETHODIMP CVMR7AllocatorPresenter::GetMinIdealVideoSize(LONG* lpWidth, LONG* lpHeight)
{
    return E_NOTIMPL;
}

STDMETHODIMP CVMR7AllocatorPresenter::GetMaxIdealVideoSize(LONG* lpWidth, LONG* lpHeight)
{
    return E_NOTIMPL;
}

STDMETHODIMP CVMR7AllocatorPresenter::SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect)
{
    return E_NOTIMPL;   // we have our own method for this
}

STDMETHODIMP CVMR7AllocatorPresenter::GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect)
{
    CopyRect(lpSRCRect, CRect(CPoint(0, 0), m_nativeVideoSize));
    CopyRect(lpDSTRect, &m_videoRect);
    // DVD Nav. bug workaround fix
    GetNativeVideoSize(&lpSRCRect->right, &lpSRCRect->bottom, nullptr, nullptr);
    return S_OK;
}

STDMETHODIMP CVMR7AllocatorPresenter::GetAspectRatioMode(DWORD* lpAspectRatioMode)
{
    if (lpAspectRatioMode) {
        *lpAspectRatioMode = AM_ARMODE_STRETCHED;
    }
    return S_OK;
}

STDMETHODIMP CVMR7AllocatorPresenter::SetAspectRatioMode(DWORD AspectRatioMode)
{
    return E_NOTIMPL;
}

STDMETHODIMP CVMR7AllocatorPresenter::SetVideoClippingWindow(HWND hwnd)
{
    return E_NOTIMPL;
}

STDMETHODIMP CVMR7AllocatorPresenter::RepaintVideo(HWND hwnd, HDC hdc)
{
    return E_NOTIMPL;
}

STDMETHODIMP CVMR7AllocatorPresenter::DisplayModeChanged()
{
    return E_NOTIMPL;
}

STDMETHODIMP CVMR7AllocatorPresenter::GetCurrentImage(BYTE** lpDib)
{
    return E_NOTIMPL;
}

STDMETHODIMP CVMR7AllocatorPresenter::SetBorderColor(COLORREF Clr)
{
    return E_NOTIMPL;
}

STDMETHODIMP CVMR7AllocatorPresenter::GetBorderColor(COLORREF* lpClr)
{
    if (lpClr) {
        *lpClr = 0;
    }
    return S_OK;
}

STDMETHODIMP CVMR7AllocatorPresenter::SetColorKey(COLORREF Clr)
{
    return E_NOTIMPL;
}

STDMETHODIMP CVMR7AllocatorPresenter::GetColorKey(COLORREF* lpClr)
{
    return E_NOTIMPL;
}
