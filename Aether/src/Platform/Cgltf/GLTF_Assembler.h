#pragma once

#include "Aether/Importer/GLBAssembler.h"
#include <string>

namespace Aether {

	class GLTF_Assembler : public GLBAssembler
	{
	public:
		virtual Ref<ParsedScene> Import(FileData data) override;
	};
}
