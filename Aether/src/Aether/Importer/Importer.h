#pragma once

#include "Aether/Core/Base.h"
#include "Tools/GLTFConverter/GLTFConverter.h"

namespace Aether {

    struct Animator
    {
        UUID skeleton;
        std::vector<UUID> clips;
    };

    struct RegisteredScene
    {
        std::vector<UUID> meshIDs;
        std::vector<UUID> matIDs;
        std::vector<UUID> sheetIDs;
        std::vector<Animator> animators;
        Ref<SceneHierarchy> hierarchy;
    };

    class AETHER_API Importer
    {
    public:
        void Init();
        void Shutdown();

        Ref<ParsedScene> ImportScene(const std::string& path);
        RegisteredScene UploadScene(const Ref<ParsedScene>& sceneData);

        std::string ImportText(const std::string& path);

    private:
        GLTFConverter m_Converter;
    };
}