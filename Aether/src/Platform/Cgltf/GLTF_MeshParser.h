#include "Aether/Importer/MeshParser.h"
#include <vector>

namespace Aether {
    class GLTF_MeshParser : public MeshParser
    {
    public:
        Ref<ParsedMeshInfo> Parsing(void* data) override;
    };
}