#pragma once
#include <Aether.h>
#include <glm/glm.hpp>
#include <vector>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include "Aether/Scene/Scene.h"

// Jolt
#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/RegisterTypes.h>

namespace Layers {
    static constexpr JPH::ObjectLayer STATIC  = 0;
    static constexpr JPH::ObjectLayer DYNAMIC = 1;
    static constexpr JPH::uint NUM_LAYERS     = 2;
}

namespace BPLayers {
    static constexpr JPH::BroadPhaseLayer STATIC  { 0 };
    static constexpr JPH::BroadPhaseLayer DYNAMIC { 1 };
    static constexpr JPH::uint NUM_LAYERS          = 2;
}

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    JPH::uint GetNumBroadPhaseLayers() const override { return BPLayers::NUM_LAYERS; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        return layer == Layers::STATIC ? BPLayers::STATIC : BPLayers::DYNAMIC;
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    { return layer == BPLayers::STATIC ? "STATIC" : "DYNAMIC"; }
#endif
};

class ObjVsBPFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer obj, JPH::BroadPhaseLayer bp) const override
    {
        if (obj == Layers::STATIC)  return bp == BPLayers::DYNAMIC;
        if (obj == Layers::DYNAMIC) return true;
        return false;
    }
};

class ObjVsObjFilter final : public JPH::ObjectLayerPairFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override
    {
        if (a == Layers::STATIC)  return b == Layers::DYNAMIC;
        if (a == Layers::DYNAMIC) return true;
        return false;
    }
};

class GameLayer : public Aether::Layer
{
public:
    GameLayer();
    virtual ~GameLayer() = default;

    virtual void Attach() override;
    virtual void Detach() override;
    virtual void Update(Aether::Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Aether::Event& event) override;

private:
    void LoadModelAsync(const std::vector<std::string>& args);
    void AddEntity(const std::vector<std::string>& args);
    void DrainParseQueue();
    void RegisterPhysicsBody(Entity transformEntity, Aether::UUID colliderMeshID, bool isDynamic = true);

    void DrawHierarchyPanel();
    void DrawEntityNode(Entity entity);
    void DrawScenePanel();
    void DrawAnimationPanel();
    void DrawLightingPanel();

private:
    Aether::Scene m_Scene;
    Aether::EditorCamera m_Camera;

    Aether::Ref<Aether::Shader> m_ShadowShader;
    Aether::Ref<Aether::Shader> m_MainShader;
    Aether::Ref<Aether::Shader> m_VolShader;

    std::vector<Aether::UUID> m_Meshes;
    std::vector<Aether::UUID> m_Animators;

    Entity m_LightEntity = Null_Entity;
    Entity m_SelectedEntity = Null_Entity;

    std::queue<Aether::ParsedScene> m_CompletedParses;
    std::mutex m_ParseMutex;

    uint32_t m_LightIdx = 0;

    float m_VolDensity = 0.03f;
    float m_VolIntensity = 1.0f;
    int   m_VolSteps = 64;
    float m_ShadowBias = 0.00001f;

    bool  m_AutoRotate = false;
    float m_RotationSpeed = 1.0f;

    int m_BindMeshIndex = -1;
    int m_BindAnimatorIndex = -1;

    JPH::PhysicsSystem m_PhysicsSystem;
    JPH::TempAllocatorImpl* m_TempAllocator = nullptr;
    JPH::JobSystemThreadPool* m_JobSystem     = nullptr;
    BPLayerInterfaceImpl m_BPLayerInterface;
    ObjVsBPFilter m_ObjVsBPFilter;
    ObjVsObjFilter m_ObjVsObjFilter;

    struct PhysicsEntry
    {
        JPH::BodyID bodyID;
        uint8_t     enabled = 0;
    };
    std::unordered_map<Entity, PhysicsEntry>  m_PhysicsBodies; 
    bool m_PhysIsDynamic = false;
    int     m_PhysNodeIdx = -1; 
    int     m_PhysMeshIdx = -1; 
    bool    m_PhysicsRunning = false;
};