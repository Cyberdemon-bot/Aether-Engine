#pragma once

#include "Aether/Assets/Sound.h"
#include "soloud_wav.h"
#include <string>

namespace Aether {

    class SoLoudSound : public Sound
    {
    public:
        SoLoudSound(const std::string& path)
        {
            SoLoud::result res = m_Wav.load(path.c_str());
            AE_CORE_ASSERT(res == SoLoud::SO_NO_ERROR, "SoLoudSound: failed to load '{0}'", path);
        }

        ~SoLoudSound() = default; // SoLoud::Wav cleans itself up

        virtual void* GetNativeHandle() override { return &m_Wav; }

    private:
        SoLoud::Wav m_Wav;
    };
}