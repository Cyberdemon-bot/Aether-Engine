#include "Aether/Audio/AudioAPI.h"
#include "soloud.h"
#include "soloud_wav.h"
#include <unordered_map>
#include <vector>

namespace Aether {
    struct AudioSource
    {
        SoLoud::Wav sound;
        AudioType type = AudioType::Audio2D;
        Audio3DConfig config;
        AudioState state;
        int handle;

        AudioSource() = default;
        AudioSource(const AudioSource&) = delete;
        AudioSource& operator=(const AudioSource&) = delete;
        AudioSource(AudioSource&&) noexcept = default;
        AudioSource& operator=(AudioSource&&) noexcept = default;
    };

    class SoLoudAPI : public AudioAPI
    {
    public:
        virtual void Init() override;
        virtual void Shutdown() override;

        virtual void CreateSource(UUID sourceID, AudioType type, const std::string& path) override;
        virtual void DestroySource(UUID sourceID) override;
        virtual void Play(UUID sourceID) override;
        virtual void Pause(UUID sourceID) override;
        virtual void Stop(UUID sourceID) override;

        virtual void SetGlobalVolume(float value) override { soloud.setGlobalVolume(value); }
        virtual void SetMaxActiveSource(uint32_t value) override { soloud.setMaxActiveVoiceCount(value); }

        virtual void SetVolume(UUID sourceID, float value) override;
        virtual void SetPan(UUID sourceID, float value) override;
        virtual void SetLooping(UUID sourceID, bool value) override;
        virtual void SetPlaybackSpeed(UUID sourceID, float value) override;
        virtual void Seek(UUID sourceID, float value) override;

        //3d only
        virtual void SetSpeedSound(float value) override { soloud.set3dSoundSpeed(value); }
        virtual void SetPosition(UUID sourceID, const glm::vec3& position) override;
        virtual void SetVelocity(UUID sourceID, const glm::vec3& velocity) override;
        virtual void SetDistance(UUID sourceID, float minDist, float maxDist) override;

        virtual void UpdateListener(const AudioListener& listener) override;
    private:
        SoLoud::Soloud soloud;
        std::unordered_map<UUID, AudioSource> m_Sources;
    };
}