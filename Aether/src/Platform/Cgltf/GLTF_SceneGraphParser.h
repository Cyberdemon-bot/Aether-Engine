#include "Aether/Importer/SceneGraphParser.h"
#include <cgltf.h>

namespace Aether {

    class GLTF_SceneGraphParser : public SceneGraphParser
    {
    public:
        Ref<SceneHierarchy> Parsing(void* data) override;

    private: 
        void ParseNode(cgltf_data* gltf, cgltf_node* node, Ref<SceneHierarchy> out ,int parentNodeIdx);
    };
}