#include "aepch.h"
#include "Aether/Scene/SceneSerializer.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Scene/Component.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Assets/Material.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Physics/PhysicsSystem.h"
// #include "Aether/Scripting/ScriptEngine.h"

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

template<typename T>
static T YGet(const YAML::Node& node, const char* key, T def = T{})
{
    auto n = node[key];
    if (!n) return def;
    try { return n.as<T>(); }
    catch (...) { return def; }
}


static EntitySnapshot PrefabToSnapshot(
    const Prefab&  prefab,
    Scene& scene,
    Entity entity,
    int scriptIndex)
{
    EntitySnapshot s;

    // --- Identity (always present) ---
    s.ID  = (uint64_t)scene.GetComponent<IDComponent>(entity).ID;

    if (prefab.tag.IsExits)
        s.Tag = prefab.tag.data.Tag;

    // Parent UUID from raw hierarchy data
    if (prefab.hierarchy.IsExits)
    {
        Entity parent = prefab.hierarchy.data.parent;
        if (parent != Null_Entity && scene.IsValid(parent))
            s.ParentID = (uint64_t)scene.GetComponent<IDComponent>(parent).ID;
    }

    // --- Transform ---
    if (prefab.transform.IsExits)
    {
        s.Translation = prefab.transform.data.Translation;
        s.Rotation    = prefab.transform.data.Rotation;
        s.Scale       = prefab.transform.data.Scale;
    }

    // --- Mesh ---
    if (prefab.mesh.IsExits)
    {
        const auto& m = prefab.mesh.data;
        s.hasMesh    = true;
        s.MeshID     = std::stoull(AssetUUID<Mesh>(m.Mesh));
        s.ShowBounds = m.ShowBounds;

        if (Sheet* sheet = AssetManager::GetAsset<Sheet>(m.Sheet))
        {
            s.MaterialIDs.reserve(sheet->BaseHandles.size());
            for (size_t i = 0; i < sheet->BaseHandles.size(); ++i)
                s.MaterialIDs.push_back(std::stoull(AssetUUID<Material>(sheet->BaseHandles[i])));

            s.OverrideMaterialIDs.reserve(sheet->OverrideHandles.size());
            for (size_t i = 0; i < sheet->OverrideHandles.size(); ++i)
                s.OverrideMaterialIDs.push_back(std::stoull(AssetUUID<Material>(sheet->OverrideHandles[i])));
        }
    }

    // --- Animator ---
    if (prefab.animator.IsExits)
    {
        const auto& a = prefab.animator.data;
        s.hasAnimator   = true;
        s.SkeletonID    = std::stoull(AssetUUID<Skeleton>(a.Skeleton));
        s.ActiveClipIdx = a.ActiveClipIdx;
        s.CurrentTime   = a.CurrentTime;
        s.Speed         = a.Speed;
        s.IsPlaying     = a.IsPlaying;
        s.Loop          = a.Loop;
        s.ClipIDs.reserve(a.Clips.size());
        for (auto& h : a.Clips)
            s.ClipIDs.push_back(std::stoull(AssetUUID<Clip>(h)));
    }

    // --- Light ---
    if (prefab.light.IsExits)
    {
        const auto& cfg = prefab.light.data.Config;
        s.hasLight      = true;
        s.LightType     = (int)cfg.type;
        s.LightPosition  = cfg.position;
        s.LightDirection = cfg.direction;
        s.LightColor    = cfg.color;
        s.Intensity     = cfg.intensity;
        s.Range         = cfg.range;
        s.InnerCone     = cfg.innerCone;
        s.OuterCone     = cfg.outerCone;
    }

    // --- Camera ---
    if (prefab.camera.IsExits)
    {
        const auto& c      = prefab.camera.data;
        s.hasCamera        = true;
        s.CameraPrimary    = c.Primary;
        s.FixedAspectRatio = c.FixedAspectRatio;
        s.ProjectionType   = (int)c.Camera.GetProjectionType();
        s.PerspectiveFOV   = c.Camera.GetPerspectiveVerticalFOV();
        s.PerspectiveNear  = c.Camera.GetPerspectiveNearClip();
        s.PerspectiveFar   = c.Camera.GetPerspectiveFarClip();
        s.OrthoSize        = c.Camera.GetOrthographicSize();
        s.OrthoNear        = c.Camera.GetOrthographicNearClip();
        s.OrthoFar         = c.Camera.GetOrthographicFarClip();
    }

    // --- Collider ---
    if (prefab.collider.IsExits)
    {
        const auto& c = prefab.collider.data;
        s.hasCollider = true;
        s.ColliderVisible = c.Visible;
        s.ColliderShape = (int)c.Shape;
        s.ColliderSize = c.Size;
        s.ColliderOffset = c.ColliderOffset;
        s.ColliderType = (int)c.Type;
        s.ColliderFriction = c.Friction;
        s.ColliderRestitution = c.Restitution;
        s.ColliderMass = c.Mass;
        s.ColliderIsSensor = c.IsSensor; 
    }

    // if (scene.HasComponent<AudioSourceComponent>(entity))
    // {
    //     s.hasAudio  = true;
    //     // s.SourceID  = (uint64_t)scene.GetComponent<AudioSourceComponent>(entity).SourceID;
    // }

    if (prefab.boneAttach.IsExits)
    {
        const auto& b     = prefab.boneAttach.data;
        s.hasBoneAttachment = true;
        s.JointName         = b.JointName;
        s.AffectChild       = b.affectChild;
        if (b.AnimatorEntity != Null_Entity && scene.IsValid(b.AnimatorEntity))
            s.AnimatorEntityID = (uint64_t)scene.GetComponent<IDComponent>(b.AnimatorEntity).ID;
    }

    // if (prefab.script.IsExits)
    // {
    //     s.hasScript    = true;
    //     s.ScriptActive = prefab.script.data.IsActive;
    //     s.ScriptIndex  = scriptIndex;
    // }

    return s;
}

static void EmitEntitySnapshot(YAML::Emitter& out, const EntitySnapshot& s)
{
    out << YAML::BeginMap;

    out << YAML::Key << "IDComponent" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "ID" << YAML::Value << s.ID;
    out << YAML::EndMap;

    out << YAML::Key << "TagComponent" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Tag" << YAML::Value << s.Tag;
    out << YAML::EndMap;

    out << YAML::Key << "TransformComponent" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Translation" << YAML::Value << s.Translation;
    out << YAML::Key << "Rotation"    << YAML::Value << s.Rotation;
    out << YAML::Key << "Scale"       << YAML::Value << s.Scale;
    out << YAML::EndMap;

    out << YAML::Key << "HierarchyComponent" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "ParentID" << YAML::Value << s.ParentID;
    out << YAML::EndMap;

    if (s.hasMesh)
    {
        out << YAML::Key << "MeshComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "MeshID"     << YAML::Value << s.MeshID;
        out << YAML::Key << "ShowBounds" << YAML::Value << s.ShowBounds;
        out << YAML::Key << "Materials"  << YAML::Value << YAML::BeginSeq;
        for (auto id : s.MaterialIDs) out << id;
        out << YAML::EndSeq;
        out << YAML::Key << "OverrideMaterials" << YAML::Value << YAML::BeginSeq;
        for (auto id : s.OverrideMaterialIDs) out << id;
        out << YAML::EndSeq;
        out << YAML::EndMap;
    }

    if (s.hasAnimator)
    {
        out << YAML::Key << "AnimatorComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "SkeletonID"    << YAML::Value << s.SkeletonID;
        out << YAML::Key << "ActiveClipIdx" << YAML::Value << s.ActiveClipIdx;
        out << YAML::Key << "CurrentTime"   << YAML::Value << s.CurrentTime;
        out << YAML::Key << "Speed"         << YAML::Value << s.Speed;
        out << YAML::Key << "IsPlaying"     << YAML::Value << s.IsPlaying;
        out << YAML::Key << "Loop"          << YAML::Value << s.Loop;
        out << YAML::Key << "Clips"         << YAML::Value << YAML::BeginSeq;
        for (auto id : s.ClipIDs) out << id;
        out << YAML::EndSeq;
        out << YAML::EndMap;
    }

    if (s.hasLight)
    {
        out << YAML::Key << "LightComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Type"      << YAML::Value << s.LightType;
        out << YAML::Key << "Position"  << YAML::Value << s.LightPosition;
        out << YAML::Key << "Direction" << YAML::Value << s.LightDirection;
        out << YAML::Key << "Color"     << YAML::Value << s.LightColor;
        out << YAML::Key << "Intensity" << YAML::Value << s.Intensity;
        out << YAML::Key << "Range"     << YAML::Value << s.Range;
        out << YAML::Key << "InnerCone" << YAML::Value << s.InnerCone;
        out << YAML::Key << "OuterCone" << YAML::Value << s.OuterCone;
        out << YAML::EndMap;
    }

    if (s.hasCamera)
    {
        out << YAML::Key << "CameraComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Primary"          << YAML::Value << s.CameraPrimary;
        out << YAML::Key << "FixedAspectRatio" << YAML::Value << s.FixedAspectRatio;
        out << YAML::Key << "ProjectionType"   << YAML::Value << s.ProjectionType;
        out << YAML::Key << "PerspectiveFOV"   << YAML::Value << s.PerspectiveFOV;
        out << YAML::Key << "PerspectiveNear"  << YAML::Value << s.PerspectiveNear;
        out << YAML::Key << "PerspectiveFar"   << YAML::Value << s.PerspectiveFar;
        out << YAML::Key << "OrthoSize"        << YAML::Value << s.OrthoSize;
        out << YAML::Key << "OrthoNear"        << YAML::Value << s.OrthoNear;
        out << YAML::Key << "OrthoFar"         << YAML::Value << s.OrthoFar;
        out << YAML::EndMap;
    }

    if (s.hasCollider)
    {
        out << YAML::Key << "ColliderComponent" << YAML::Value << YAML::BeginMap; 
        out << YAML::Key << "Visible"        << YAML::Value << s.ColliderVisible;
        out << YAML::Key << "Shape"          << YAML::Value << s.ColliderShape;
        out << YAML::Key << "Size"           << YAML::Value << s.ColliderSize;
        out << YAML::Key << "ColliderOffset" << YAML::Value << s.ColliderOffset;
        out << YAML::Key << "ColliderType" << YAML::Value << s.ColliderType;
        out << YAML::Key << "ColliderFriction" << YAML::Value << s.ColliderFriction;
        out << YAML::Key << "ColliderRestitution" << YAML::Value << s.ColliderRestitution;
        out << YAML::Key << "ColliderMass" << YAML::Value << s.ColliderMass;
        out << YAML::Key << "ColliderIsSensor" << YAML::Value << s.ColliderIsSensor;
        out << YAML::EndMap;
    }

    // if (s.hasAudio)
    // {
    //     out << YAML::Key << "AudioSourceComponent" << YAML::Value << YAML::BeginMap;
    //     out << YAML::Key << "SourceID" << YAML::Value << s.SourceID;
    //     out << YAML::EndMap;
    // }

    if (s.hasBoneAttachment)
    {
        out << YAML::Key << "BoneAttachmentComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "AnimatorEntityID" << YAML::Value << s.AnimatorEntityID;
        out << YAML::Key << "JointName"        << YAML::Value << s.JointName;
        out << YAML::Key << "AffectChild"      << YAML::Value << s.AffectChild;
        out << YAML::EndMap;
    }

    // if (s.hasScript)
    // {
    //     out << YAML::Key << "ScriptComponent" << YAML::Value << YAML::BeginMap;
    //     out << YAML::Key << "ScriptIndex"  << YAML::Value << s.ScriptIndex;
    //     out << YAML::Key << "IsActive"     << YAML::Value << s.ScriptActive;
    //     out << YAML::EndMap;
    // }

    out << YAML::EndMap;
}

bool SceneSerializer::Serialize(Scene& scene, const std::string& path, const std::string& sceneName)
{
    // std::vector<std::string> scriptSources;
    // std::unordered_map<std::string, int> scriptDedup;

    // auto scriptView = scene.View<ScriptComponent>();
    // for (auto entity : scriptView)
    // {
    //     auto& sc  = scene.GetComponent<ScriptComponent>(entity);
    //     if (!sc.ScriptHandle.IsValid()) continue;
    //
    //     std::string raw = ScriptEngine::GetRaw(sc.ScriptHandle);
    //     if (raw.empty()) continue;
    //
    //     if (scriptDedup.find(raw) == scriptDedup.end())
    //     {
    //         scriptDedup[raw] = (int)scriptSources.size();
    //         scriptSources.push_back(raw);
    //     }
    // }

    std::vector<EntitySnapshot> snapshots;

    std::function<void(Entity)> walk = [&](Entity e)
    {
        if (!scene.IsValid(e)) return;

        Prefab prefab = scene.ExportPrefab(e);

        int scriptIndex = -1;
        // if (prefab.script.IsExits && prefab.script.data.ScriptHandle.IsValid())
        // {
        //     std::string raw = ScriptEngine::GetRaw(prefab.script.data.ScriptHandle);
        //     auto it = scriptDedup.find(raw);
        //     if (it != scriptDedup.end()) scriptIndex = it->second;
        // }

        snapshots.push_back(PrefabToSnapshot(prefab, scene, e, scriptIndex));

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

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Scene" << YAML::Value << sceneName;

    // std::string scriptPath = path + ".script";
    // out << YAML::Key << "ScriptBundle" << YAML::Value << scriptPath;
    // out << YAML::Key << "ScriptCount"  << YAML::Value << (int)scriptSources.size();

    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
    for (const auto& s : snapshots)
        EmitEntitySnapshot(out, s);
    out << YAML::EndSeq;
    out << YAML::EndMap;

    if (!out.good())
    {
        AE_CORE_ERROR("SceneSerializer::Serialize: emitter error: {0}", out.GetLastError());
        return false;
    }

    {
        std::ofstream file(path);
        if (!file)
        {
            AE_CORE_ERROR("SceneSerializer::Serialize: cannot open '{0}'", path);
            return false;
        }
        file << out.c_str();
        if (!file.good()) return false;
    }

    // {
    //     std::ofstream sf(scriptPath, std::ios::binary);
    //     if (!sf)
    //     {
    //         AE_CORE_ERROR("SceneSerializer::Serialize: cannot open script bundle '{0}'", scriptPath);
    //         return false;
    //     }
    //
    //     sf << scriptSources.size() << "\n";
    //     for (const auto& src : scriptSources)
    //     {
    //         sf << src.size() << "\n";
    //         sf.write(src.data(), (std::streamsize)src.size());
    //         sf << "\n";
    //     }
    //
    //     if (!sf.good())
    //     {
    //         AE_CORE_ERROR("SceneSerializer::Serialize: write error on script bundle '{0}'", scriptPath);
    //         return false;
    //     }
    // }

    AE_CORE_INFO("SceneSerializer: saved '{0}' — {1} entities",
                 path, snapshots.size());
    return true;
}

static void ReadTransform(const YAML::Node& n, EntitySnapshot& s)
{
    s.Translation = YGet<glm::vec3>(n, "Translation");
    s.Rotation    = YGet<glm::quat>(n, "Rotation", glm::quat(1,0,0,0));
    s.Scale       = YGet<glm::vec3>(n, "Scale", glm::vec3(1.0f));
}

static void ReadMesh(const YAML::Node& n, EntitySnapshot& s)
{
    s.hasMesh    = true;
    s.MeshID     = YGet<uint64_t>(n, "MeshID");
    s.ShowBounds = YGet<bool>(n, "ShowBounds", false);
    if (auto mats = n["Materials"])
        for (const auto& m : mats)
            s.MaterialIDs.push_back(m.as<uint64_t>(0));
    if (auto overrideMats = n["OverrideMaterials"])
        for (const auto& m : overrideMats)
            s.OverrideMaterialIDs.push_back(m.as<uint64_t>(0));
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
    s.hasCollider     = true;
    s.ColliderVisible = YGet<bool>(n, "Visible", false);
    s.ColliderShape   = YGet<int>(n, "Shape", 0);
    s.ColliderSize    = YGet<glm::vec3>(n, "Size");
    s.ColliderOffset  = YGet<glm::vec3>(n, "ColliderOffset");
    s.ColliderType  = YGet<int>(n, "ColliderType", 0);
    s.ColliderFriction  = YGet<float>(n, "ColliderFriction", 0);
    s.ColliderRestitution  = YGet<float>(n, "ColliderRestitution", 0);
    s.ColliderMass  = YGet<float>(n, "ColliderMass", 0);
    s.ColliderIsSensor  = YGet<bool>(n, "ColliderIsSensor", false);
}

// static void ReadAudio(const YAML::Node& n, EntitySnapshot& s)
// {
//     s.hasAudio = true;
//     s.SourceID = YGet<uint64_t>(n, "SourceID");
// }

static void ReadBoneAttachment(const YAML::Node& n, EntitySnapshot& s)
{
    s.hasBoneAttachment = true;
    s.AnimatorEntityID  = YGet<uint64_t>(n, "AnimatorEntityID");
    s.JointName         = YGet<std::string>(n, "JointName");
    s.AffectChild       = YGet<bool>(n, "AffectChild", true);
}

// static void ReadScript(const YAML::Node& n, EntitySnapshot& s)
// {
//     s.hasScript    = true;
//     s.ScriptIndex  = YGet<int>(n, "ScriptIndex", -1);
//     s.ScriptActive = YGet<bool>(n, "IsActive", true);
// }

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
    snapshot.ScriptSources.clear();

    // if (auto scriptBundleNode = root["ScriptBundle"])
    // {
    //     std::string scriptPath = scriptBundleNode.as<std::string>("");
    //     int expectedCount = YGet<int>(root, "ScriptCount", 0);
    //
    //     if (!scriptPath.empty() && expectedCount > 0)
    //     {
    //         std::ifstream sf(scriptPath, std::ios::binary);
    //         if (!sf)
    //         {
    //             AE_CORE_WARN("SceneSerializer::Deserialize: cannot open script bundle '{0}' — scripts will be missing", scriptPath);
    //         }
    //         else
    //         {
    //             int count = 0;
    //             sf >> count;
    //             sf.ignore(1);
    //
    //             snapshot.ScriptSources.reserve(count);
    //             for (int i = 0; i < count; ++i)
    //             {
    //                 size_t len = 0;
    //                 sf >> len;
    //                 sf.ignore(1);
    //
    //                 std::string src(len, '\0');
    //                 sf.read(src.data(), (std::streamsize)len);
    //                 sf.ignore(1);
    //
    //                 if (!sf)
    //                 {
    //                     AE_CORE_ERROR("SceneSerializer::Deserialize: read error in script bundle at index {0}", i);
    //                     break;
    //                 }
    //                 snapshot.ScriptSources.push_back(std::move(src));
    //             }
    //
    //             if ((int)snapshot.ScriptSources.size() != expectedCount)
    //                 AE_CORE_WARN("SceneSerializer::Deserialize: expected {0} scripts, got {1}", expectedCount, snapshot.ScriptSources.size());
    //         }
    //     }
    // }

    auto entities = root["Entities"];
    if (!entities || !entities.IsSequence()) return true; 

    for (const auto& node : entities)
    {
        EntitySnapshot s;

        if (auto id = node["IDComponent"])
            s.ID = YGet<uint64_t>(id, "ID");
        if (auto tag = node["TagComponent"])
            s.Tag = YGet<std::string>(tag, "Tag", "Entity");
        if (auto hier = node["HierarchyComponent"])
            s.ParentID = YGet<uint64_t>(hier, "ParentID", 0);

        if (auto t = node["TransformComponent"])     ReadTransform(t, s);
        if (auto n = node["MeshComponent"])          ReadMesh(n, s);
        if (auto n = node["AnimatorComponent"])      ReadAnimator(n, s);
        if (auto n = node["LightComponent"])         ReadLight(n, s);
        if (auto n = node["CameraComponent"])        ReadCamera(n, s);
        if (auto n = node["ColliderComponent"])      ReadCollider(n, s);
        // if (auto n = node["AudioSourceComponent"])   ReadAudio(n, s);
        if (auto n = node["BoneAttachmentComponent"]) ReadBoneAttachment(n, s);
        // if (auto n = node["ScriptComponent"])        ReadScript(n, s);

        snapshot.Entities.push_back(std::move(s));
    }

    AE_CORE_INFO("SceneSerializer: loaded '{0}' — {1} entities",
                 path, snapshot.Entities.size());
    return true;
}

bool SceneSerializer::DeserializeInto(const std::string& path, Scene& scene)
{
    SceneSnapshot snapshot;
    if (!Deserialize(path, snapshot)) return false;

    // std::vector<Handle<Bytecode>> compiledScripts;
    // compiledScripts.reserve(snapshot.ScriptSources.size());
    // for (const auto& src : snapshot.ScriptSources)
    // {
    //     Handle<Bytecode> bh = ScriptEngine::LoadScriptSource(src);
    //     compiledScripts.push_back(bh);
    // }

    std::unordered_map<uint64_t, Entity> idMap;
    idMap.reserve(snapshot.Entities.size());

    for (const auto& s : snapshot.Entities)
    {
        Entity e = scene.CreateEntity(s.Tag, UUID(s.ID));
        idMap[s.ID] = e;

        auto& t   = scene.GetComponent<TransformComponent>(e);
        t.Translation = s.Translation;
        t.Rotation    = glm::normalize(s.Rotation);
        t.Scale       = s.Scale;
        scene.MarkDirty(e);
    }

    for (const auto& s : snapshot.Entities)
    {
        Entity e = idMap[s.ID];

        // Hierarchy
        if (s.ParentID != 0)
        {
            auto it = idMap.find(s.ParentID);
            if (it != idMap.end())
                scene.MakeParent(e, it->second);
            else
                AE_CORE_WARN("SceneSerializer: entity {0} references unknown parent {1}", s.ID, s.ParentID);
        }

        // Mesh
        if (s.hasMesh)
        {
            auto& c      = scene.AddComponent<MeshComponent>(e);
            c.Mesh       = AssetManager::GetHandle(UUID(s.MeshID));
            c.ShowBounds = s.ShowBounds;

            Handle<Asset> sheetHandle = AssetManager::CreateAsset<Sheet>(UUID());
            Sheet* sheet = AssetManager::GetAsset<Sheet>(sheetHandle);

            sheet->Resize(s.MaterialIDs.size());
            for (size_t i = 0; i < s.MaterialIDs.size(); ++i)
                sheet->SetDefault(i, AssetManager::GetHandle(UUID(s.MaterialIDs[i])));

            for (size_t i = 0; i < s.OverrideMaterialIDs.size() && i < sheet->GetSize(); ++i)
            {
                if (s.OverrideMaterialIDs[i] != 0)
                    sheet->SetOverride(i, AssetManager::GetHandle(UUID(s.OverrideMaterialIDs[i])));
            }

            c.Sheet = sheetHandle;
        }

        // Animator
        if (s.hasAnimator)
        {
            auto& c         = scene.AddComponent<AnimatorComponent>(e);
            c.Skeleton      = AssetManager::GetHandle(UUID(s.SkeletonID));
            c.ActiveClipIdx = s.ActiveClipIdx;
            c.CurrentTime   = s.CurrentTime;
            c.Speed         = s.Speed;
            c.IsPlaying     = s.IsPlaying;
            c.Loop          = s.Loop;

            for (auto clipID : s.ClipIDs)
                c.Clips.push_back(AssetManager::GetHandle(UUID(clipID)));

            // Eagerly build animation cache for the active clip.
            if (!c.Clips.empty())
            {
                auto  rigModule = AnimationSystem::GetModule<RigModule>();
                auto* skelAsset = AssetManager::GetAsset<Skeleton>(c.Skeleton);
                auto* clipAsset = AssetManager::GetAsset<Clip>(c.Clips[0]);
                if (rigModule && skelAsset && clipAsset)
                    c.Cache = rigModule->CreateCache(clipAsset->GetHandle());
            }
        }

        // Light
        if (s.hasLight)
        {
            auto& c          = scene.AddComponent<LightComponent>(e);
            c.Config.type      = (LightType)s.LightType;
            c.Config.position  = s.LightPosition;
            c.Config.direction = s.LightDirection;
            c.Config.color     = s.LightColor;
            c.Config.intensity = s.Intensity;
            c.Config.range     = s.Range;
            c.Config.innerCone = s.InnerCone;
            c.Config.outerCone = s.OuterCone;
        }

        // Camera
        if (s.hasCamera)
        {
            auto& c            = scene.AddComponent<CameraComponent>(e);
            c.Primary          = s.CameraPrimary;
            c.FixedAspectRatio = s.FixedAspectRatio;
            c.Camera.SetProjectionType((SceneCamera::ProjectionType)s.ProjectionType);
            c.Camera.SetPerspectiveVerticalFOV(s.PerspectiveFOV);
            c.Camera.SetPerspectiveNearClip(s.PerspectiveNear);
            c.Camera.SetPerspectiveFarClip(s.PerspectiveFar);
            c.Camera.SetOrthographicSize(s.OrthoSize);
            c.Camera.SetOrthographicNearClip(s.OrthoNear);
            c.Camera.SetOrthographicFarClip(s.OrthoFar);
        }

        // Collider
        if (s.hasCollider)
        {
            auto& c          = scene.AddComponent<ColliderComponent>(e);
            c.Visible        = s.ColliderVisible;
            c.Shape          = (ColliderShape)s.ColliderShape;
            c.Size           = s.ColliderSize;
            c.ColliderOffset = s.ColliderOffset;
            c.Type = (MotionType)s.ColliderType;
            c.Friction = s.ColliderFriction;
            c.Restitution = s.ColliderRestitution;
            c.Mass = s.ColliderMass;
            c.IsSensor = s.ColliderIsSensor;
        }

        // Audio
        // if (s.hasAudio)
        // {
        //     auto& c   = scene.AddComponent<AudioSourceComponent>(e);
        //     //c.SourceID = UUID(s.SourceID);
        // }

        // Bone attachment — AnimatorEntity resolved in pass 3.
        if (s.hasBoneAttachment)
        {
            auto& c       = scene.AddComponent<BoneAttachmentComponent>(e);
            c.JointName   = s.JointName;
            c.affectChild = s.AffectChild;
        }

        // Script
        // if (s.hasScript && s.ScriptIndex >= 0 && s.ScriptIndex < (int)compiledScripts.size())
        // {
        //     Handle<Bytecode> bh = compiledScripts[s.ScriptIndex];
        //     if (bh.IsValid())
        //     {
        //         Handle<ScriptInstance> instance = ScriptEngine::CreateInstance(&scene, e, bh);
        //         auto& sc        = scene.AddComponent<ScriptComponent>(e, instance);
        //         sc.IsActive     = s.ScriptActive;
        //         ScriptEngine::SetActiveStage(instance, s.ScriptActive);
        //     }
        //     else
        //     {
        //         AE_CORE_WARN("SceneSerializer: invalid bytecode for entity {0} (ScriptIndex={1})", s.ID, s.ScriptIndex);
        //     }
        // }
    }

    for (const auto& s : snapshot.Entities)
    {
        if (!s.hasBoneAttachment || s.AnimatorEntityID == 0) continue;
        Entity e   = idMap[s.ID];
        auto   it  = idMap.find(s.AnimatorEntityID);
        if (it != idMap.end())
            scene.GetComponent<BoneAttachmentComponent>(e).AnimatorEntity = it->second;
        else
            AE_CORE_WARN("SceneSerializer: BoneAttachment on {0} references unknown entity {1}", s.ID, s.AnimatorEntityID);
    }

    return true;
}

}