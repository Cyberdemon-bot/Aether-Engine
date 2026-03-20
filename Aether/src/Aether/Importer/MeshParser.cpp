#include "aepch.h"
#include "Aether/Importer/Importer.h"
#include "Platform/Cgltf/GLTF_MeshParser.h"
#include "Aether/Core/Assert.h"

namespace Aether {
    Ref<MeshParser> MeshParser::Create()
    {
        switch (Importer::GetAPI())
		{
			case ImporterAPI::API::None:    AE_CORE_ASSERT(false, "None is currently not supported api for ImporterAPI!"); return nullptr;
			case ImporterAPI::API::Cgltf:  return CreateRef<GLTF_MeshParser>();
		}

		AE_CORE_ASSERT(false, "Unknown ImporterAPI!");
		return nullptr;
    }
}