#include "Aether/Importer/Importer.h"
#include "Platform/Cgltf/GLTF_SceneGraphParser.h"

namespace Aether {
    Ref<SceneGraphParser> SceneGraphParser::Create()
    {
        switch (Importer::GetAPI())
		{
			case ImporterAPI::API::None:    AE_CORE_ASSERT(false, "None is currently not supported api for ImporterAPI!"); return nullptr;
			case ImporterAPI::API::Cgltf:  return CreateRef<GLTF_SceneGraphParser>();
		}

		AE_CORE_ASSERT(false, "Unknown ImporterAPI!");
		return nullptr;
    }
}