#pragma once
#include "Aether/Core/UUID.h"
#include <string> 
#include <unordered_map>

namespace Aether {
    class AssetsRegister
    {
    public:
        static std::string Get(UUID key);
        static UUID Register(const std::string& name);
        static bool Exists(UUID key);

    private:
        static std::unordered_map<UUID, std::string>& GetMap();
    };
}