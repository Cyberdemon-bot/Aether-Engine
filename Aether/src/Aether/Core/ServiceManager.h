#pragma once
#include "Aether/Core/Base.h"
#include "Aether/Core/Log.h"

namespace Aether {

    class AETHER_API ServiceManager
    {
    private:
        template<typename T>
        struct ServiceHolder 
        {
            static inline T* s_Instance = nullptr;
        };

    public:
        template<typename T>
        static T* GetService()
        {
            T* service = ServiceHolder<T>::s_Instance;
            return service;
        }

    private:
        template<typename T>
        static void Provide(T* service)
        {
            ServiceHolder<T>::s_Instance = service;
        }

        template<typename T>
        static void ShutdownService()
        {
            ServiceHolder<T>::s_Instance = nullptr;
        }

        friend class Application;
    };

}