#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Aether/Core/Base.h"

namespace Aether
{
    class Scene;

    struct EntitySnapshot
    {
        uint64_t ID = 0;
        std::string Tag;
        uint64_t ParentID = 0;  

        glm::vec3 Translation = glm::vec3(0.0f);
        glm::quat Rotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 Scale       = glm::vec3(1.0f);

        bool hasMesh = false;
        uint64_t MeshID = 0;
        bool ShowBounds = false;
        std::vector<uint64_t> MaterialIDs;

        bool hasAnimator = false;
        uint64_t SkeletonID = 0;
        std::vector<uint64_t> ClipIDs;
        int ActiveClipIdx = 0;
        float CurrentTime = 0.0f;
        float Speed = 1.0f;
        bool IsPlaying = true;
        bool Loop = true;

        bool hasLight = false;
        int LightType = 0;
        glm::vec3 LightPosition = glm::vec3(0.0f);
        glm::vec3 LightDirection = glm::vec3(0.0f, -1.0f, 0.0f);
        glm::vec3 LightColor = glm::vec3(1.0f);
        float Intensity = 1.0f;
        float Range = 10.0f;
        float InnerCone = 0.0f;
        float OuterCone = 0.785f;

        bool hasCamera = false;
        bool CameraPrimary = true;
        bool FixedAspectRatio = false;
        int ProjectionType = 0;
        float PerspectiveFOV = 45.0f;
        float PerspectiveNear = 0.01f;
        float PerspectiveFar = 1000.0f;
        float OrthoSize = 10.0f;
        float OrthoNear = -1.0f;
        float OrthoFar = 1.0f;

        bool hasCollider = false;
        bool ColliderVisible = false;
        int ColliderShape = 0;
        glm::vec3 ColliderSize = glm::vec3(0.0f);
        glm::vec3 ColliderOffset = glm::vec3(0.0f);
        int ColliderType = 0;
        float ColliderMass = 1.0f;
        float ColliderFriction = 0.5f;
        float ColliderRestitution = 0.0f;
        bool ColliderIsSensor = false;

        bool hasAudio = false;
        uint64_t SourceID = 0;

        bool hasBoneAttachment = false;
        uint64_t AnimatorEntityID = 0;
        std::string JointName;
        bool AffectChild = true;

        // Script component.
        // ScriptIndex is an index into SceneSnapshot::ScriptSources (-1 = none).
        // Multiple entities sharing identical source code share the same index.
        bool hasScript = false;
        bool ScriptActive = true;
        int  ScriptIndex = -1;
    };

    struct SceneSnapshot
    {
        std::string                  SceneName;
        std::vector<EntitySnapshot>  Entities;

        // Deduplicated raw Lua source strings.
        // Each entry maps to one .script file slot written by the serializer.
        std::vector<std::string>     ScriptSources;
    };

    class AETHER_API SceneSerializer
    {
    public:
        // Serialize scene to <path> (YAML) and <path>.script (script bundle).
        // sceneName is embedded in the YAML header.
        static bool Serialize(Scene& scene, const std::string& path, const std::string& sceneName = "Untitled");

        // Deserialize YAML at <path> (and its companion .script file) into snapshot.
        static bool Deserialize(const std::string& path, SceneSnapshot& snapshot);

        // Deserialize directly into a live Scene.
        static bool DeserializeInto(const std::string& path, Scene& scene);
    };
}