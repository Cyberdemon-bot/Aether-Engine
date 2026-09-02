#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Assets/BatchRegisterResult.h"
#include "Tools/GLTFConverter/GLTFConverter.h"

namespace Aether {

    struct ParsedScene
    {
        Ref<SceneHierarchy> scene;
        Ref<CreateInfoList> result;
    };

    struct RegisteredScene
    {
        BatchRegisterResult assets;
        Ref<SceneHierarchy> hierarchy;
    };

    class AETHER_API Importer
    {
    public:
        void Init();
        void Shutdown();

        ParsedScene ImportScene(const std::string& path);
        RegisteredScene UploadScene(const ParsedScene& sceneData);
        UUID ImportAudio(const std::string& path);

        std::string ImportText(const std::string& path);

    private:
        GLTFConverter m_Converter;
    };
}