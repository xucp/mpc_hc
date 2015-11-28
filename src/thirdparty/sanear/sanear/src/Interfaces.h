#pragma once

#include <comdef.h>
#include <ocidl.h>

namespace SaneAudioRenderer
{
    struct __declspec(uuid("6A5DD5AC-A1BF-4FB7-9C04-2A89970F8132"))
    ISettings : IUnknown
    {
        STDMETHOD_(UINT32, GetSerial)() = 0;

        enum
        {
            OUTPUT_DEVICE_BUFFER_MIN_MS = 60,
            OUTPUT_DEVICE_BUFFER_MAX_MS = 1000,
            OUTPUT_DEVICE_BUFFER_DEFAULT_MS = 200,
        };
        STDMETHOD(SetOuputDevice)(LPCWSTR pDeviceId, BOOL bExclusive, UINT32 uBufferMS) = 0;
        STDMETHOD(GetOuputDevice)(LPWSTR* ppDeviceId, BOOL* pbExclusive, UINT32* puBufferMS) = 0;

        STDMETHOD_(void, SetAllowBitstreaming)(BOOL bAllowBitstreaming) = 0;
        STDMETHOD_(void, GetAllowBitstreaming)(BOOL* pbAllowBitstreaming) = 0;

        STDMETHOD_(void, SetCrossfeedEnabled)(BOOL bEnable) = 0;
        STDMETHOD_(void, GetCrossfeedEnabled)(BOOL* pbEnabled) = 0;

        enum
        {
            CROSSFEED_CUTOFF_FREQ_MIN = 300,
            CROSSFEED_CUTOFF_FREQ_MAX = 2000,
            CROSSFEED_CUTOFF_FREQ_CMOY = 700,
            CROSSFEED_CUTOFF_FREQ_JMEIER = 650,
            CROSSFEED_LEVEL_MIN = 10,
            CROSSFEED_LEVEL_MAX = 150,
            CROSSFEED_LEVEL_CMOY = 60,
            CROSSFEED_LEVEL_JMEIER = 95,
        };
        STDMETHOD(SetCrossfeedSettings)(UINT32 uCutoffFrequency, UINT32 uCrossfeedLevel) = 0;
        STDMETHOD_(void, GetCrossfeedSettings)(UINT32* puCutoffFrequency, UINT32* puCrossfeedLevel) = 0;
    };
    _COM_SMARTPTR_TYPEDEF(ISettings, __uuidof(ISettings));

    struct __declspec(uuid("03481710-D73E-4674-839F-03EDE2D60ED8"))
    ISpecifyPropertyPages2 : ISpecifyPropertyPages
    {
        STDMETHOD(CreatePage)(const GUID& guid, IPropertyPage** ppPage) = 0;
    };
    _COM_SMARTPTR_TYPEDEF(ISpecifyPropertyPages2, __uuidof(ISpecifyPropertyPages2));
}
