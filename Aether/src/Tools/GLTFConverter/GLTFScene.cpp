#include "aepch.h"
#include "GLTFConverter.h"

#include <cgltf.h>

namespace Aether {

    // Kept for backward compatibility (per your instruction) - logic is
    // unchanged from the original GLTF_SceneGraphParser, just moved here
    // and de-virtualized. See the code-smell note next to SceneHierarchy's
    // declaration in GLTFConverter.h.
    Ref<SceneHierarchy> GLTFConverter::ParseSceneGraph(cgltf_data* gltf)
    {
        Ref<SceneHierarchy> out = CreateRef<SceneHierarchy>();
        if (!gltf) return out;

        cgltf_scene* scene = gltf->scene ? gltf->scene : (gltf->scenes_count > 0 ? &gltf->scenes[0] : nullptr);
        if (!scene) return out;

        std::unordered_set<cgltf_node*> jointNodes;
        for (size_t i = 0; i < gltf->skins_count; i++)
            for (size_t j = 0; j < gltf->skins[i].joints_count; j++)
                jointNodes.insert(gltf->skins[i].joints[j]);

        for (size_t i = 0; i < scene->nodes_count; i++)
            ParseNode(gltf, scene->nodes[i], out, -1, jointNodes);

        return out;
    }

    void GLTFConverter::ParseNode(cgltf_data* gltf, cgltf_node* node, const Ref<SceneHierarchy>& out,
                                   int parentNodeIdx, const std::unordered_set<cgltf_node*>& jointNodes)
    {
        if (!node) return;
        if (jointNodes.count(node)) return;

        Node sceneNode;
        sceneNode.name = node->name ? node->name : ("Node_" + std::to_string(out->nodes.size()));

        if (node->has_translation || node->has_rotation || node->has_scale)
        {
            if (node->has_translation)
                sceneNode.translation = glm::vec3(node->translation[0], node->translation[1], node->translation[2]);
            if (node->has_rotation)
                sceneNode.rotation = glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);
            if (node->has_scale)
                sceneNode.scale = glm::vec3(node->scale[0], node->scale[1], node->scale[2]);
        }
        else if (node->has_matrix)
        {
            glm::mat4 m;
            for (int col = 0; col < 4; col++)
                for (int row = 0; row < 4; row++)
                    m[col][row] = node->matrix[col * 4 + row];

            sceneNode.translation = glm::vec3(m[3]);
            sceneNode.scale = glm::vec3(
                glm::length(glm::vec3(m[0])),
                glm::length(glm::vec3(m[1])),
                glm::length(glm::vec3(m[2])));
            glm::mat3 rotMat(
                glm::vec3(m[0]) / sceneNode.scale.x,
                glm::vec3(m[1]) / sceneNode.scale.y,
                glm::vec3(m[2]) / sceneNode.scale.z);
            sceneNode.rotation = glm::quat_cast(rotMat);
        }

        if (node->mesh) sceneNode.meshIdx = (int)(node->mesh - gltf->meshes);
        if (node->skin) sceneNode.animatorIdx = (int)(node->skin - gltf->skins);

        int myIdx = (int)out->nodes.size();
        out->nodes.push_back(sceneNode);

        if (parentNodeIdx == -1) out->roots.push_back(myIdx);
        else out->nodes[parentNodeIdx].children.push_back(myIdx);

        for (size_t i = 0; i < node->children_count; i++)
            ParseNode(gltf, node->children[i], out, myIdx, jointNodes);
    }
}