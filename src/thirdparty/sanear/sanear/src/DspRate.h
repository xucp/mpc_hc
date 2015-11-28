#pragma once

#include "DspBase.h"

#include <soxr.h>

namespace SaneAudioRenderer
{
    class DspRate final
        : public DspBase
    {
    public:

        DspRate() = default;
        DspRate(const DspRate&) = delete;
        DspRate& operator=(const DspRate&) = delete;
        ~DspRate();

        void Initialize(bool variable, uint32_t inputRate, uint32_t outputRate, uint32_t channels);

        std::wstring Name() override { return L"Rate"; }

        bool Active() override;

        void Process(DspChunk& chunk) override;
        void Finish(DspChunk& chunk) override;

    private:

        void DestroyBackend();

        soxr_t m_soxr = nullptr;
        uint32_t m_inputRate = 0;
        uint32_t m_outputRate = 0;
        uint32_t m_channels = 0;
    };
}
