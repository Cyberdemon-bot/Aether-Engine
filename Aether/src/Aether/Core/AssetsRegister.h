#pragma once
#include "Aether/Core/UUID.h"
#include <string> 
#include <unordered_map>

namespace Aether {
    class AssetsRegister
    {
    public:
        static UUID Get(const std::string& key);
        static UUID Register(const std::string& key);
        static bool Exists(const std::string& key);

    private:
        static std::unordered_map<std::string, UUID>& GetMap();
    };
}