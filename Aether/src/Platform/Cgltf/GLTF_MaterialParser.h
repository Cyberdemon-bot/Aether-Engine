#include "Aether/Importer/MaterialParser.h"

namespace Aether {
    class GLTF_MaterialParser : public MaterialParser
    {
    public:
        Ref<ParsedMaterialInfo> Parsing(void* data) override;
    };
}