#include "aepch.h" 
#include "AssetsRegister.h"

namespace Aether {
    std::unordered_map<std::string, UUID>& AssetsRegister::GetMap()
    {
        static std::unordered_map<std::string, UUID> s_Map;
        return s_Map;
    }

    UUID AssetsRegister::Get(const std::string& key)
    {
        auto& map = GetMap();
        if (map.find(key) == map.end()) 
        {
            AE_CORE_ERROR("Key '{0}' has not registered yet!", key);
            return 0; 
        }
        return map[key];
    }

    UUID AssetsRegister::Register(const std::string& key)
    {
        if (Exists(key)) {
            AE_CORE_WARN("Key '{0}' already exists! Returning existing UUID.", key);
            return Get(key);
        }

        UUID newID = UUID(); 
        GetMap()[key] = newID;
        return newID;
    }

    void AssetsRegister::Register(const std::string& key, UUID id)
    {
        if (Exists(key)) {
            AE_CORE_WARN("Key '{0}' already exists! Returning existing UUID.", key);
            return;
        }

        GetMap()[key] = id;
    }

    bool AssetsRegister::Exists(const std::string& key)
    {
        auto& map = GetMap();
        return map.find(key) != map.end();
    }
}