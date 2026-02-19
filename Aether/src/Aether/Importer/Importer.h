#include "Aether/Core/Base.h"
#include "Aether/Importer/ImporterAPI.h"

namespace Aether {
    class AETHER_API Importer
    {
    public: 
        static ParsedScene Import(const std::string& path) {
            return s_ImporterAPI->Import(path);
        }

        static RegisteredScene Upload(const ParsedScene& sceneData) {
            return s_ImporterAPI->Upload(sceneData);
        }

        static ImporterAPI::API GetAPI() { return ImporterAPI::GetAPI(); }
    private:
        static Scope<ImporterAPI> s_ImporterAPI;
    };
}