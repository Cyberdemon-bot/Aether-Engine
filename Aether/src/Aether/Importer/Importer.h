#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Importer/ImporterAPI.h"

namespace Aether {
    class AETHER_API Importer
    {
    public: 
        static Ref<ParsedScene> Import(const std::string& path, bool createCache = false, const char* cacheName = "") {
            return s_ImporterAPI->Import(path, createCache, cacheName);
        }

        static Ref<ParsedScene> ImportCache(const char* cacheName) {
            return s_ImporterAPI->ImportCache(cacheName);
        }

        static RegisteredScene Upload(const Ref<ParsedScene>& sceneData) {
            return s_ImporterAPI->Upload(sceneData);
        }

        static ImporterAPI::API GetAPI() { return ImporterAPI::GetAPI(); }
    private:
        static Scope<ImporterAPI> s_ImporterAPI;
    };
}