#include "aepch.h" 
#include "AssetsRegister.h"

namespace Aether {
    std::unordered_map<UUID, std::string>& AssetsRegister::GetMap()
    {
        static std::unordered_map<UUID, std::string> s_Map;
        return s_Map;
    }

    std::string AssetsRegister::Get(UUID key)
    {
        auto& map = GetMap();
        if (map.find(key) == map.end()) 
        {
            AE_CORE_ERROR("Key '{0}' has not registered yet!", uint64_t(key));
            return ""; 
        }
        return map[key];
    }

    UUID AssetsRegister::Register(const std::string& name, UUID id)
    {
        GetMap()[id] = name;
        return id;
    }

    bool AssetsRegister::Exists(UUID key)
    {
        auto& map = GetMap();
        return map.find(key) != map.end();
    }
}