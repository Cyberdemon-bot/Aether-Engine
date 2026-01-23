#pragma once
#include "aepch.h"
#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"

namespace Aether {

    class AssetsRegister
    {
    public:
        static UUID Get(const std::string& key)
        {
            auto& map = GetMap();
            if (map.find(key) == map.end()) 
            {
                AE_CORE_ERROR("Key has not regist yet!");
                return 0; 
            }
            return map[key];
        }

        static UUID Register(const std::string& key)
        {
            UUID newID = UUID();
            GetMap()[key] = newID;
            return newID;
        }

        static bool Exists(const std::string& key)
        {
            auto& map = GetMap();
            return map.find(key) != map.end();
        }

    private:
        static std::unordered_map<std::string, UUID>& GetMap()
        {
            static std::unordered_map<std::string, UUID> s_Map;
            return s_Map;
        }
    };
}