#pragma once

#include <array>
#include "Aether/Core/Base.h"

namespace Aether {

    enum class ServiceType : uint8_t
    {
        Renderer = 0,
        AudioSystem,
        AssetManager,
        AnimationSystem,
        ScriptEngine,
        JobSystem,
        PhysicsSystem,
        Count
    };

    class Renderer;
    class AudioSystem;
    class AssetManager;
    class AnimationSystem;
    class ScriptEngine;
    class JobSystem;
    class PhysicsSystem;

    template<typename T> struct GetServiceType;
    template<> struct GetServiceType<Renderer> { static constexpr ServiceType value = ServiceType::Renderer; };
    template<> struct GetServiceType<AudioSystem> { static constexpr ServiceType value = ServiceType::AudioSystem; };
    template<> struct GetServiceType<AssetManager> { static constexpr ServiceType value = ServiceType::AssetManager; };
    template<> struct GetServiceType<AnimationSystem> { static constexpr ServiceType value = ServiceType::AnimationSystem; };
    template<> struct GetServiceType<ScriptEngine> { static constexpr ServiceType value = ServiceType::ScriptEngine; };
    template<> struct GetServiceType<JobSystem> { static constexpr ServiceType value = ServiceType::JobSystem; };
    template<> struct GetServiceType<PhysicsSystem> { static constexpr ServiceType value = ServiceType::PhysicsSystem; };

    template<typename T>
    inline constexpr ServiceType GetServiceType_v = GetServiceType<T>::value;

    class AETHER_API ServiceManager
    {
    public:
        static void Init();
        static void Shutdown();

        template<typename T>
        static void Provide(T* service)
        {
            constexpr ServiceType type = GetServiceType_v<T>;
            uint8_t idx = static_cast<uint8_t>(type);
            s_Services[idx] = static_cast<void*>(service);
        }

        template<typename T>
        static T* GetService()
        {
            constexpr ServiceType type = GetServiceType_v<T>;
            uint8_t idx = static_cast<uint8_t>(type);
            return static_cast<T*>(s_Services[idx]);
        }

    private:
        static std::array<void*, static_cast<size_t>(ServiceType::Count)> s_Services;
    };

}