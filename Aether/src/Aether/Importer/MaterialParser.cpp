#include "aepch.h"
#include "Aether/Importer/Importer.h"
#include "Platform/Cgltf/GLTF_MaterialParser.h"
#include "Aether/Core/Assert.h"

namespace Aether {
    Ref<MaterialParser> MaterialParser::Create()
    {
        switch (GLBAssembler::GetAPI())
		{
			case GLBAssembler::API::None:    AE_CORE_ASSERT(false, "None is currently not supported api for GLBAssembler!"); return nullptr;
			case GLBAssembler::API::Cgltf:  return CreateRef<GLTF_MaterialParser>();
		}

		AE_CORE_ASSERT(false, "Unknown GLBAssembler!");
		return nullptr;
    }
}
