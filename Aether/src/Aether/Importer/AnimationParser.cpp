#include "aepch.h"
#include "Aether/Importer/Importer.h"
#include "Platform/Cgltf/GLTF_AnimationParser.h"
#include "Aether/Core/Assert.h"

namespace Aether {

    Ref<AnimationParser> AnimationParser::Create()
    {
        switch (Importer::Get_GLB_API())
		{
			case GLBAssembler::API::None:    AE_CORE_ASSERT(false, "None is currently not supported api for GLBAssembler!"); return nullptr;
			case GLBAssembler::API::Cgltf:  return CreateRef<GLTF_AnimationParser>();
		}

		AE_CORE_ASSERT(false, "Unknown GLBAssembler!");
		return nullptr;
    }

}