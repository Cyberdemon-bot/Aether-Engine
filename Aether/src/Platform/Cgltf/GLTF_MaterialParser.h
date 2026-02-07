#include "Aether/Importer/MaterialParser.h"
#include <vector>

namespace Aether {
    class GLTF_MaterialParser : public MaterialParser
    {
    public:
        Ref<ParsedMaterialInfo> Parsing(void* data) override;
    };
}