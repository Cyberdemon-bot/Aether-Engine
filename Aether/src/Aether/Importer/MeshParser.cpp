#include "aepch.h"
#include "Aether/Importer/Importer.h"
#include "Platform/Cgltf/GLTF_MeshParser.h"
#include "Aether/Core/Assert.h"

namespace Aether {
    Ref<MeshParser> MeshParser::Create()
    {
        switch (LegacyAssembler::GetAPI())
		{
			case LegacyAssembler::API::None:    AE_CORE_ASSERT(false, "None is currently not supported api for LegacyAssembler!"); return nullptr;
			case LegacyAssembler::API::Cgltf:  return CreateRef<GLTF_MeshParser>();
		}

		AE_CORE_ASSERT(false, "Unknown LegacyAssembler!");
		return nullptr;
    }
}