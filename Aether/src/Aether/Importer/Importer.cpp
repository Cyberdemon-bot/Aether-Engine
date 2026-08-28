#include "aepch.h"
#include "Aether/Importer/Importer.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/FileSystem/FileSystem.h"
#include "Aether/Assets/AssetRegister.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Core/Assert.h"

namespace Aether {

    static void CalculateSkinnedBoundsAOS(AMeshCreateInfo& spec, std::vector<glm::mat4>& poseMats)
    {
        if (spec.streams.empty() || spec.streams[0].VertexCount == 0 || poseMats.empty())
        {
            spec.hasAnimatedBounds = false;
            return;
        }

        const SkinnedVertex* verts = static_cast<const SkinnedVertex*>(spec.streams[0].Data);
        uint32_t vertexCount = spec.streams[0].VertexCount;

        glm::vec3 boundsMin(FLT_MAX);
        glm::vec3 boundsMax(-FLT_MAX);

        for (uint32_t i = 0; i < vertexCount; i++)
        {
            const auto& v = verts[i];
            glm::vec4 skinnedPos =
                poseMats[v.Joints.x] * glm::vec4(v.Position, 1.0f) * v.Weights.x +
                poseMats[v.Joints.y] * glm::vec4(v.Position, 1.0f) * v.Weights.y +
                poseMats[v.Joints.z] * glm::vec4(v.Position, 1.0f) * v.Weights.z +
                poseMats[v.Joints.w] * glm::vec4(v.Position, 1.0f) * v.Weights.w;

            boundsMin = glm::min(boundsMin, glm::vec3(skinnedPos));
            boundsMax = glm::max(boundsMax, glm::vec3(skinnedPos));
        }

        spec.animatedBoundsMin = boundsMin;
        spec.animatedBoundsMax = boundsMax;
        spec.hasAnimatedBounds = true;
    }

    void Importer::Init()
    {
        // GLTFConverter is stateless (no API selection, nothing to spin up) -
        // kept as a no-op only to preserve the existing Init/Shutdown call
        // sites elsewhere in the engine's startup sequence.
    }

    void Importer::Shutdown()
    {
    }

    Ref<ParsedScene> Importer::ImportScene(const std::string& path)
    {
        auto* fs = ServiceManager::GetService<FileSystem>();
        fs->RegisterPath(path);
        fs->CommitRegistry();
        auto handle = fs->Open(path);
        ParsedScene scene = m_Converter.Import(fs->GetBytes(handle));
        fs->Close(handle);
        return CreateRef<ParsedScene>(std::move(scene));
    }

    std::string Importer::ImportText(const std::string& path)
    {
        auto* fs = ServiceManager::GetService<FileSystem>();
        fs->RegisterPath(path);
        fs->CommitRegistry();
        auto handle = fs->Open(path);
        auto temp = fs->GetBytes(handle);
        std::string res(reinterpret_cast<const char*>(temp.bytes), temp.size);
        fs->Close(handle);
        return res;
    }

    RegisteredScene Importer::UploadScene(const Ref<ParsedScene>& sceneData)
    {
        RegisteredScene res;

        auto* assetRegister = ServiceManager::GetService<AssetRegister>();
        auto* assetManager = ServiceManager::GetService<AssetManager>();
        auto  animSystem = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();

        for (const auto& img : sceneData->Images)
            assetRegister->Register<AImage>(img);

        for (const auto& mat : sceneData->Materials)
        {
            assetRegister->Register<AMaterial>(mat);
            res.matIDs.push_back(mat.id);
        }

        for (const auto& skel : sceneData->Skeletons)
        {
            assetRegister->Register<ASkeleton>(skel);
            res.animators.push_back({});
            res.animators.back().skeleton = skel.id;
        }

        for (const auto& clip : sceneData->Clips)
        {
            assetRegister->Register<AClip>(clip);
            for (auto& animator : res.animators)
            {
                if (animator.skeleton == clip.skeleton)
                {
                    animator.clips.push_back(clip.id);
                    break;
                }
            }
        }

        for (size_t meshIdx = 0; meshIdx < sceneData->Meshes.size(); meshIdx++)
        {
            const AMeshCreateInfo& meshInfo = sceneData->Meshes[meshIdx];

            int rigIdx = -1;
            if (sceneData->Hierarchy)
            {
                for (const auto& node : sceneData->Hierarchy->nodes)
                {
                    if (node.meshIdx == (int)meshIdx && node.animatorIdx >= 0)
                    {
                        rigIdx = node.animatorIdx;
                        break;
                    }
                }
            }

            std::vector<UUID> sheetMatIDs;
            std::vector<SubMesh> submeshes;
            submeshes.reserve(meshInfo.submeshes.size());

            for (const auto& subInfo : meshInfo.submeshes)
            {
                SubMesh sm = subInfo;
                if (subInfo.MaterialIdx >= 0 && subInfo.MaterialIdx < (int)sceneData->Materials.size())
                {
                    sheetMatIDs.push_back(sceneData->Materials[subInfo.MaterialIdx].id);
                    sm.MaterialIdx = (int)sheetMatIDs.size() - 1;
                }
                else sm.MaterialIdx = -1;
                submeshes.push_back(sm);
            }

            UUID sheetID = assetRegister->Register<ASheet>(ASheetCreateInfo{
                UUID(),
                meshInfo.debugName + "_Sheet",
                sheetMatIDs.data(),
                (uint32_t)sheetMatIDs.size()
            });
            res.sheetIDs.push_back(sheetID);

            AMeshCreateInfo spec = meshInfo;
            spec.submeshes = std::span<const SubMesh>(submeshes);

            if (rigIdx >= 0 && rigIdx < (int)sceneData->Skeletons.size())
            {
                UUID skeletonID = sceneData->Skeletons[rigIdx].id;
                auto asset = assetManager->GetAsset<ASkeleton>(skeletonID);
                if (asset)
                {
                    auto handle = asset->m_Handle;
                    std::vector<glm::mat4> poseMats(asset->m_JointCount);
                    animSystem->GetRestPoseMatrices(handle, poseMats.data(), poseMats.size());
                    CalculateSkinnedBoundsAOS(spec, poseMats);
                }
            }

            assetRegister->Register<AMesh>(spec);
            res.meshIDs.push_back(meshInfo.id);
        }

        res.hierarchy = sceneData->Hierarchy;
        return res;
    }
}