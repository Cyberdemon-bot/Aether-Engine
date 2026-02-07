#include "Aether/Importer/ImporterAPI.h"

namespace Aether {
    class Importer
    {
    public: 
        static ParsedScene Import(const std::string& path) {
            return s_ImporterAPI->Import(path);
        }

        static RegisteredScene Upload(const ParsedScene& sceneData, UUID shaderID) {
            return s_ImporterAPI->Upload(sceneData, shaderID);
        }

        static ImporterAPI::API GetAPI() { return ImporterAPI::GetAPI(); }
    private:
        static Scope<ImporterAPI> s_ImporterAPI;
    };
}