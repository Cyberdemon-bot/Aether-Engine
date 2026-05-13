#include "aepch.h"
#include "Aether/Scene/SceneSerializer.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Scene/Component.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Physics/PhysicsSystem.h"

#include <yaml-cpp/yaml.h>
#include <glm/gtx/quaternion.hpp>
#include <fstream>
#include <unordered_map>

namespace YAML
{
    template<> struct convert<glm::vec3>
    {
        static Node encode(const glm::vec3& v)
        {
            Node n; n.push_back(v.x); n.push_back(v.y); n.push_back(v.z);
            n.SetStyle(EmitterStyle::Flow); return n;
        }
        static bool decode(const Node& n, glm::vec3& v)
        {
            if (!n.IsSequence() || n.size() != 3) return false;
            v.x = n[0].as<float>(); v.y = n[1].as<float>(); v.z = n[2].as<float>();
            return true;
        }
    };

    template<> struct convert<glm::vec4>
    {
        static Node encode(const glm::vec4& v)
        {
            Node n; n.push_back(v.x); n.push_back(v.y); n.push_back(v.z); n.push_back(v.w);
            n.SetStyle(EmitterStyle::Flow); return n;
        }
        static bool decode(const Node& n, glm::vec4& v)
        {
            if (!n.IsSequence() || n.size() != 4) return false;
            v.x = n[0].as<float>(); v.y = n[1].as<float>();
            v.z = n[2].as<float>(); v.w = n[3].as<float>();
            return true;
        }
    };

    template<> struct convert<glm::quat>
    {
        static Node encode(const glm::quat& q)
        {
            Node n; n.push_back(q.x); n.push_back(q.y); n.push_back(q.z); n.push_back(q.w);
            n.SetStyle(EmitterStyle::Flow); return n;
        }
        static bool decode(const Node& n, glm::quat& q)
        {
            if (!n.IsSequence() || n.size() != 4) return false;
            q.x = n[0].as<float>(); q.y = n[1].as<float>();
            q.z = n[2].as<float>(); q.w = n[3].as<float>();
            return true;
        }
    };
}

namespace Aether
{

    static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
    { out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq; return out; }

    static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
    { out << YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq; return out; }

    static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::quat& q)
    { out << YAML::Flow << YAML::BeginSeq << q.x << q.y << q.z << q.w << YAML::EndSeq; return out; }

    template<typename TAsset>
    static std::string AssetUUID(Handle<Asset> handle)
    {
        if (!handle.IsValid()) return "0";
        auto* asset = AssetManager::GetAsset<TAsset>(handle);
        if (!asset) return "0";
        return std::to_string((uint64_t)asset->id);
    }

    static void SerializeIDComponent(YAML::Emitter& out, Scene& scene, Entity e)
    {
        auto& c = scene.GetComponent<IDComponent>(e);
        out << YAML::Key << "IDComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "ID" << YAML::Value << (uint64_t)c.ID;
        out << YAML::EndMap;
    }

    static void SerializeTagComponent(YAML::Emitter& out, Scene& scene, Entity e)
    {
        auto& c = scene.GetComponent<TagComponent>(e);
        out << YAML::Key << "TagComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Tag" << YAML::Value << c.Tag;
        out << YAML::EndMap;
    }

    static void SerializeTransformComponent(YAML::Emitter& out, Scene& scene, Entity e)
    {
        auto& c = scene.GetComponent<TransformComponent>(e);
        out << YAML::Key << "TransformComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Translation" << YAML::Value << c.Translation;
        out << YAML::Key << "Rotation" << YAML::Value << c.Rotation;
        out << YAML::Key << "Scale" << YAML::Value << c.Scale;
        out << YAML::EndMap;
    }

    static void SerializeHierarchyComponent(YAML::Emitter& out, Scene& scene, Entity e)
    {
        auto& c = scene.GetComponent<HierarchyComponent>(e);
        uint64_t parentID = 0;
        if (c.parent != Null_Entity && scene.IsValid(c.parent))
            parentID = (uint64_t)scene.GetComponent<IDComponent>(c.parent).ID;
        out << YAML::Key << "HierarchyComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "ParentID" << YAML::Value << parentID;
        out << YAML::EndMap;
    }

    static void SerializeMeshComponent(YAML::Emitter& out, Scene& scene, Entity e)
    {
        auto& c = scene.GetComponent<MeshComponent>(e);
        out << YAML::Key << "MeshComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "MeshID" << YAML::Value << AssetUUID<Mesh>(c.Mesh);
        out << YAML::Key << "ShowBounds" << YAML::Value << c.ShowBounds;
        out << YAML::Key << "Materials"  << YAML::Value << YAML::BeginSeq;
        for (size_t i = 0; i < c.Materials.BaseHandles.size(); ++i)
            out << AssetUUID<Material>(c.Materials.BaseHandles[i]);
        out << YAML::EndSeq;
        out << YAML::EndMap;
    }

    static void SerializeAnimatorComponent(YAML::Emitter& out, Scene& scene, Entity e)
    {
        auto& c = scene.GetComponent<AnimatorComponent>(e);
        out << YAML::Key << "AnimatorComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "SkeletonID" << YAML::Value << AssetUUID<Skeleton>(c.Skeleton);
        out << YAML::Key << "ActiveClipIdx" << YAML::Value << c.ActiveClipIdx;
        out << YAML::Key << "CurrentTime" << YAML::Value << c.CurrentTime;
        out << YAML::Key << "Speed" << YAML::Value << c.Speed;
        out << YAML::Key << "IsPlaying" << YAML::Value << c.IsPlaying;
        out << YAML::Key << "Loop" << YAML::Value << c.Loop;
        out << YAML::Key << "Clips" << YAML::Value << YAML::BeginSeq;
        for (auto& h : c.Clips) out << AssetUUID<Clip>(h);
        out << YAML::EndSeq;
        out << YAML::EndMap;
    }

    static void SerializeLightComponent(YAML::Emitter& out, Scene& scene, Entity e)
    {
        auto& cfg = scene.GetComponent<LightComponent>(e).Config;
        out << YAML::Key << "LightComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Type" << YAML::Value << (int)cfg.type;
        out << YAML::Key << "Position" << YAML::Value << cfg.position;
        out << YAML::Key << "Direction" << YAML::Value << cfg.direction;
        out << YAML::Key << "Color" << YAML::Value << cfg.color;
        out << YAML::Key << "Intensity" << YAML::Value << cfg.intensity;
        out << YAML::Key << "Range" << YAML::Value << cfg.range;
        out << YAML::Key << "InnerCone" << YAML::Value << cfg.innerCone;
        out << YAML::Key << "OuterCone" << YAML::Value << cfg.outerCone;
        out << YAML::EndMap;
    }

    static void SerializeCameraComponent(YAML::Emitter& out, Scene& scene, Entity e)
    {
        auto& c = scene.GetComponent<CameraComponent>(e);
        out << YAML::Key << "CameraComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Primary" << YAML::Value << c.Primary;
        out << YAML::Key << "FixedAspectRatio" << YAML::Value << c.FixedAspectRatio;
        out << YAML::Key << "ProjectionType" << YAML::Value << (int)c.Camera.GetProjectionType();
        out << YAML::Key << "PerspectiveFOV" << YAML::Value << c.Camera.GetPerspectiveVerticalFOV();
        out << YAML::Key << "PerspectiveNear" << YAML::Value << c.Camera.GetPerspectiveNearClip();
        out << YAML::Key << "PerspectiveFar" << YAML::Value << c.Camera.GetPerspectiveFarClip();
        out << YAML::Key << "OrthoSize" << YAML::Value << c.Camera.GetOrthographicSize();
        out << YAML::Key << "OrthoNear" << YAML::Value << c.Camera.GetOrthographicNearClip();
        out << YAML::Key << "OrthoFar" << YAML::Value << c.Camera.GetOrthographicFarClip();
        out << YAML::EndMap;
    }

    static void SerializeColliderComponent(YAML::Emitter& out, Scene& scene, Entity e)
    {
        auto& c = scene.GetComponent<ColliderComponent>(e);
        out << YAML::Key << "ColliderComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Visible" << YAML::Value << c.Visible;
        out << YAML::Key << "Shape" << YAML::Value << (int)c.Shape;
        out << YAML::Key << "Size" << YAML::Value << c.Size;
        out << YAML::Key << "ColliderOffset" << YAML::Value << c.ColliderOffset;
        out << YAML::EndMap;
    }

    static void SerializeAudioSourceComponent(YAML::Emitter& out, Scene& scene, Entity e)
    {
        auto& c = scene.GetComponent<AudioSourceComponent>(e);
        out << YAML::Key << "AudioSourceComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "SourceID" << YAML::Value << (uint64_t)c.SourceID;
        out << YAML::EndMap;
    }

    static void SerializeBoneAttachmentComponent(YAML::Emitter& out, Scene& scene, Entity e)
    {
        auto& c = scene.GetComponent<BoneAttachmentComponent>(e);
        uint64_t animEntityID = 0;
        if (c.AnimatorEntity != Null_Entity && scene.IsValid(c.AnimatorEntity))
            animEntityID = (uint64_t)scene.GetComponent<IDComponent>(c.AnimatorEntity).ID;
        out << YAML::Key << "BoneAttachmentComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "AnimatorEntityID" << YAML::Value << animEntityID;
        out << YAML::Key << "JointName" << YAML::Value << c.JointName;
        out << YAML::Key << "AffectChild" << YAML::Value << c.affectChild;
        out << YAML::EndMap;
    }

    static void SerializeEntity(YAML::Emitter& out, Scene& scene, Entity e)
    {
        if (!scene.HasComponent<IDComponent>(e)) return;
        out << YAML::BeginMap;
        SerializeIDComponent(out, scene, e);
        SerializeTagComponent(out, scene, e);
        SerializeTransformComponent(out, scene, e);
        SerializeHierarchyComponent(out, scene, e);
        if (scene.HasComponent<MeshComponent>(e)) SerializeMeshComponent(out, scene, e);
        if (scene.HasComponent<AnimatorComponent>(e)) SerializeAnimatorComponent(out, scene, e);
        if (scene.HasComponent<LightComponent>(e)) SerializeLightComponent(out, scene, e);
        if (scene.HasComponent<CameraComponent>(e)) SerializeCameraComponent(out, scene, e);
        if (scene.HasComponent<ColliderComponent>(e)) SerializeColliderComponent(out, scene, e);
        if (scene.HasComponent<AudioSourceComponent>(e)) SerializeAudioSourceComponent(out, scene, e);
        if (scene.HasComponent<BoneAttachmentComponent>(e))SerializeBoneAttachmentComponent(out, scene, e);
        out << YAML::EndMap;
    }

    bool SceneSerializer::Serialize(Scene& scene, const std::string& path)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << "Untitled";
        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        std::function<void(Entity)> walk = [&](Entity e)
        {
            if (!scene.IsValid(e)) return;
            SerializeEntity(out, scene, e);
            Entity child = scene.GetComponent<HierarchyComponent>(e).firstChild;
            while (child != Null_Entity)
            {
                walk(child);
                child = scene.GetComponent<HierarchyComponent>(child).nextSibling;
            }
        };

        for (auto e : scene.View<HierarchyComponent>())
            if (scene.GetComponent<HierarchyComponent>(e).parent == Null_Entity)
                walk(e);

        out << YAML::EndSeq;
        out << YAML::EndMap;

        if (!out.good())
        {
            AE_CORE_ERROR("SceneSerializer::Serialize: emitter error: {0}", out.GetLastError());
            return false;
        }

        std::ofstream file(path);
        if (!file)
        {
            AE_CORE_ERROR("SceneSerializer::Serialize: cannot open '{0}'", path);
            return false;
        }
        file << out.c_str();
        return file.good();
    }
    
    template<typename T>
    static T YGet(const YAML::Node& node, const char* key, T def = T{})
    {
        auto n = node[key];
        if (!n) return def;
        try { return n.as<T>(); }
        catch (...) { return def; }
    }

    static void ReadTransform(const YAML::Node& n, EntitySnapshot& s)
    {
        s.Translation = YGet<glm::vec3>(n, "Translation");
        s.Rotation = YGet<glm::quat>(n, "Rotation", glm::quat(1,0,0,0));
        s.Scale = YGet<glm::vec3>(n, "Scale", glm::vec3(1.0f));
    }

    static void ReadMesh(const YAML::Node& n, EntitySnapshot& s)
    {
        s.hasMesh = true;
        s.MeshID = YGet<uint64_t>(n, "MeshID");
        s.ShowBounds = YGet<bool>(n, "ShowBounds", false);
        if (auto mats = n["Materials"])
            for (const auto& m : mats)
                s.MaterialIDs.push_back(m.as<uint64_t>(0));
    }

    static void ReadAnimator(const YAML::Node& n, EntitySnapshot& s)
    {
        s.hasAnimator = true;
        s.SkeletonID = YGet<uint64_t>(n, "SkeletonID");
        s.ActiveClipIdx = YGet<int>(n, "ActiveClipIdx", 0);
        s.CurrentTime = YGet<float>(n, "CurrentTime", 0.0f);
        s.Speed = YGet<float>(n, "Speed", 1.0f);
        s.IsPlaying = YGet<bool>(n, "IsPlaying", true);
        s.Loop = YGet<bool>(n, "Loop", true);
        if (auto clips = n["Clips"])
            for (const auto& c : clips)
                s.ClipIDs.push_back(c.as<uint64_t>(0));
    }

    static void ReadLight(const YAML::Node& n, EntitySnapshot& s)
    {
        s.hasLight = true;
        s.LightType = YGet<int>(n, "Type", 0);
        s.LightPosition = YGet<glm::vec3>(n, "Position");
        s.LightDirection = YGet<glm::vec3>(n, "Direction", glm::vec3(0,-1,0));
        s.LightColor = YGet<glm::vec3>(n, "Color", glm::vec3(1));
        s.Intensity = YGet<float>(n, "Intensity", 1.0f);
        s.Range = YGet<float>(n, "Range", 10.0f);
        s.InnerCone = YGet<float>(n, "InnerCone", 0.0f);
        s.OuterCone = YGet<float>(n, "OuterCone", 0.785f);
    }

    static void ReadCamera(const YAML::Node& n, EntitySnapshot& s)
    {
        s.hasCamera = true;
        s.CameraPrimary = YGet<bool>(n,  "Primary",          true);
        s.FixedAspectRatio = YGet<bool>(n,  "FixedAspectRatio", false);
        s.ProjectionType = YGet<int>(n,   "ProjectionType",   0);
        s.PerspectiveFOV = YGet<float>(n, "PerspectiveFOV",   45.0f);
        s.PerspectiveNear = YGet<float>(n, "PerspectiveNear",  0.01f);
        s.PerspectiveFar = YGet<float>(n, "PerspectiveFar",   1000.0f);
        s.OrthoSize = YGet<float>(n, "OrthoSize",        10.0f);
        s.OrthoNear = YGet<float>(n, "OrthoNear",        -1.0f);
        s.OrthoFar = YGet<float>(n, "OrthoFar",         1.0f);
    }

    static void ReadCollider(const YAML::Node& n, EntitySnapshot& s)
    {
        s.hasCollider = true;
        s.ColliderVisible = YGet<bool>(n, "Visible", false);
        s.ColliderShape = YGet<int>(n, "Shape", 0);
        s.ColliderSize = YGet<glm::vec3>(n, "Size");
        s.ColliderOffset  = YGet<glm::vec3>(n, "ColliderOffset");
    }

    static void ReadAudio(const YAML::Node& n, EntitySnapshot& s)
    {
        s.hasAudio = true;
        s.SourceID = YGet<uint64_t>(n, "SourceID");
    }

    static void ReadBoneAttachment(const YAML::Node& n, EntitySnapshot& s)
    {
        s.hasBoneAttachment = true;
        s.AnimatorEntityID = YGet<uint64_t>(n,    "AnimatorEntityID");
        s.JointName = YGet<std::string>(n, "JointName");
        s.AffectChild = YGet<bool>(n,        "AffectChild", true);
    }

    bool SceneSerializer::Deserialize(const std::string& path, SceneSnapshot& snapshot)
    {
        YAML::Node root;
        try { root = YAML::LoadFile(path); }
        catch (const YAML::Exception& e)
        {
            AE_CORE_ERROR("SceneSerializer::Deserialize: YAML parse error in '{0}': {1}", path, e.what());
            return false;
        }

        if (!root["Scene"])
        {
            AE_CORE_ERROR("SceneSerializer::Deserialize: missing 'Scene' key in '{0}'", path);
            return false;
        }

        snapshot.SceneName = root["Scene"].as<std::string>("Untitled");
        snapshot.Entities.clear();

        auto entities = root["Entities"];
        if (!entities || !entities.IsSequence()) return true; // empty scene is valid

        for (const auto& node : entities)
        {
            EntitySnapshot s;

            // --- Identity ---
            if (auto id = node["IDComponent"])
                s.ID = YGet<uint64_t>(id, "ID");
            if (auto tag = node["TagComponent"])
                s.Tag = YGet<std::string>(tag, "Tag", "Entity");
            if (auto hier = node["HierarchyComponent"])
                s.ParentID = YGet<uint64_t>(hier, "ParentID", 0);

            if (auto t = node["TransformComponent"]) ReadTransform(t, s);

            if (auto n = node["MeshComponent"]) ReadMesh(n, s);
            if (auto n = node["AnimatorComponent"]) ReadAnimator(n, s);
            if (auto n = node["LightComponent"]) ReadLight(n, s);
            if (auto n = node["CameraComponent"]) ReadCamera(n, s);
            if (auto n = node["ColliderComponent"]) ReadCollider(n, s);
            if (auto n = node["AudioSourceComponent"]) ReadAudio(n, s);
            if (auto n = node["BoneAttachmentComponent"]) ReadBoneAttachment(n, s);

            snapshot.Entities.push_back(std::move(s));
        }

        AE_CORE_INFO("SceneSerializer: loaded '{0}' — {1} entities", path, snapshot.Entities.size());
        return true;
    }

    bool SceneSerializer::DeserializeInto(const std::string& path, Scene& scene)
    {
        SceneSnapshot snapshot;
        if (!Deserialize(path, snapshot)) return false;
        std::unordered_map<uint64_t, Entity> idMap;

        for (const auto& s : snapshot.Entities)
        {
            Entity e = scene.CreateEntity(s.Tag, s.ID);  
            idMap[s.ID] = e;

            // Transform
            auto& t    = scene.GetComponent<TransformComponent>(e);
            t.Translation = s.Translation;
            t.Rotation    = glm::normalize(s.Rotation);
            t.Scale       = s.Scale;
            scene.MarkDirty(e);
        }

        for (const auto& s : snapshot.Entities)
        {
            Entity e = idMap[s.ID];

            // --- Hierarchy ---
            if (s.ParentID != 0)
            {
                auto it = idMap.find(s.ParentID);
                if (it != idMap.end())
                    scene.MakeParent(e, it->second);
                else
                    AE_CORE_WARN("SceneSerializer: entity {0} references unknown parent {1}", s.ID, s.ParentID);
            }

            // --- Mesh ---
            if (s.hasMesh)
            {
                auto& c   = scene.AddComponent<MeshComponent>(e);
                c.Mesh       = AssetManager::GetHandle(UUID(s.MeshID));
                c.ShowBounds = s.ShowBounds;
                c.Materials.Resize(s.MaterialIDs.size());
                for (size_t i = 0; i < s.MaterialIDs.size(); ++i)
                    c.Materials.SetDefault(i, AssetManager::GetHandle(UUID(s.MaterialIDs[i])));
            }

            // --- Animator ---
            if (s.hasAnimator)
            {
                auto& c     = scene.AddComponent<AnimatorComponent>(e);
                c.Skeleton  = AssetManager::GetHandle(UUID(s.SkeletonID));
                c.ActiveClipIdx = s.ActiveClipIdx;
                c.CurrentTime   = s.CurrentTime;
                c.Speed         = s.Speed;
                c.IsPlaying     = s.IsPlaying;
                c.Loop          = s.Loop;

                for (auto clipID : s.ClipIDs)
                    c.Clips.push_back(AssetManager::GetHandle(UUID(clipID)));

                if (!c.Clips.empty())
                {
                    auto  rigModule    = AnimationSystem::GetModule<RigModule>();
                    auto* skelAsset    = AssetManager::GetAsset<Skeleton>(c.Skeleton);
                    auto* clipAsset    = AssetManager::GetAsset<Clip>(c.Clips[0]);
                    if (skelAsset && clipAsset)
                        c.Cache = rigModule->CreateCache(clipAsset->GetHandle());
                }
            }

            // --- Light ---
            if (s.hasLight)
            {
                auto& c = scene.AddComponent<LightComponent>(e);
                c.Config.type = (LightType)s.LightType;
                c.Config.position = s.LightPosition;
                c.Config.direction = s.LightDirection;
                c.Config.color = s.LightColor;
                c.Config.intensity = s.Intensity;
                c.Config.range = s.Range;
                c.Config.innerCone = s.InnerCone;
                c.Config.outerCone = s.OuterCone;
            }

            // --- Camera ---
            if (s.hasCamera)
            {
                auto& c = scene.AddComponent<CameraComponent>(e);
                c.Primary = s.CameraPrimary;
                c.FixedAspectRatio = s.FixedAspectRatio;
                c.Camera.SetProjectionType((SceneCamera::ProjectionType)s.ProjectionType);
                c.Camera.SetPerspectiveVerticalFOV(s.PerspectiveFOV);
                c.Camera.SetPerspectiveNearClip(s.PerspectiveNear);
                c.Camera.SetPerspectiveFarClip(s.PerspectiveFar);
                c.Camera.SetOrthographicSize(s.OrthoSize);
                c.Camera.SetOrthographicNearClip(s.OrthoNear);
                c.Camera.SetOrthographicFarClip(s.OrthoFar);
            }

            if (s.hasCollider)
            {
                auto& c = scene.AddComponent<ColliderComponent>(e);
                c.Visible = s.ColliderVisible;
                c.Shape = (ColliderShape)s.ColliderShape;
                c.Size = s.ColliderSize;
                c.ColliderOffset = s.ColliderOffset;
            }


            // --- Audio ---
            if (s.hasAudio)
            {
                auto& c = scene.AddComponent<AudioSourceComponent>(e);
                c.SourceID = UUID(s.SourceID);
            }

            if (s.hasBoneAttachment)
            {
                auto& c = scene.AddComponent<BoneAttachmentComponent>(e);
                c.JointName = s.JointName;
                c.affectChild = s.AffectChild;
            }
        }

        for (const auto& s : snapshot.Entities)
        {
            if (!s.hasBoneAttachment || s.AnimatorEntityID == 0) continue;
            Entity e = idMap[s.ID];
            auto it = idMap.find(s.AnimatorEntityID);
            if (it != idMap.end())
                scene.GetComponent<BoneAttachmentComponent>(e).AnimatorEntity = it->second;
            else
                AE_CORE_WARN("SceneSerializer: BoneAttachment on {0} references unknown entity {1}", s.ID, s.AnimatorEntityID);
        }

        return true;
    }

} 