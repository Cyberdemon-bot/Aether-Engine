#include "Aether/Importer/MeshParser.h"

namespace Aether {
    class GLTF_MeshParser : public MeshParser
    {
    public:
        Ref<ParsedMeshInfo> Parsing(void* data) override;
    };
}