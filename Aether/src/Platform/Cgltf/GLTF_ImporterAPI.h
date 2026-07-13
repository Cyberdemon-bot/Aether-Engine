#pragma once

#include "Aether/Importer/ImporterAPI.h"
#include <string>

namespace Aether {

	class GLTF_ImporterAPI : public ImporterAPI
	{
	public:
		virtual Ref<ParsedScene> Import(const std::string& path) override;
	};
}
