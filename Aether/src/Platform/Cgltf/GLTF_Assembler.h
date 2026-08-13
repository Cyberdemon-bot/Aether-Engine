#pragma once

#include "Aether/Importer/LegacyAssembler.h"
#include <string>

namespace Aether {

	class GLTF_Assembler : public LegacyAssembler
	{
	public:
		virtual Ref<ParsedScene> Import(FileData data) override;
	};
}
