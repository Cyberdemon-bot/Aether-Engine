#pragma once

#include <typeindex>
#include <vector>

namespace Aether {
    class AETHER_API ServiceManager
    {
    public:
        static void Init();
        static void Shutdown();

        template<typename T>
        static void Provide(T* service)
        {
            s_Services[std::type_index(typeid(T))] = static_cast<void*>(service);
        }

        template<typename T>
        static T* GetService()
        {
            auto it = s_Services.find(std::type_index(typeid(T)));
            if (it != s_Services.end()) 
            {
                return static_cast<T*>(it->second);
            }
            return nullptr;
        }
    private:
        static std::unordered_map<std::type_index, void*> s_Services;
    };
}