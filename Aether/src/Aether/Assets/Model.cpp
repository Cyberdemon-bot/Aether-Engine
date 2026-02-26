#include "Aether/Assets/Model.h"

namespace Aether {

    Model::Model(Ref<Mesh> Mesh, const std::vector<Ref<Texture2D>>& Materials)
        : mesh(Mesh), materials(Materials) {}
}