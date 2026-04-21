#pragma once
#include "Aether/Core/UUID.h"
#include "Aether/Core/Base.h"
#include <string> 
#include <unordered_map>

namespace Aether {
    class AETHER_API AssetsRegister
    {
    public:
        static std::string Get(UUID key);
        static UUID Register(const std::string& name, UUID id);
        static bool Exists(UUID key);

    private:
        static std::unordered_map<UUID, std::string>& GetMap();
    };
}