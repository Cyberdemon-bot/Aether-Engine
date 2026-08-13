#include "aepch.h"
#include "Aether/Importer/Importer.h"
#include "Platform/Cgltf/GLTF_MaterialParser.h"
#include "Aether/Core/Assert.h"

namespace Aether {
    Ref<MaterialParser> MaterialParser::Create()
    {
        switch (LegacyAssembler::GetAPI())
		{
			case LegacyAssembler::API::None:    AE_CORE_ASSERT(false, "None is currently not supported api for LegacyAssembler!"); return nullptr;
			case LegacyAssembler::API::Cgltf:  return CreateRef<GLTF_MaterialParser>();
		}

		AE_CORE_ASSERT(false, "Unknown LegacyAssembler!");
		return nullptr;
    }
}
