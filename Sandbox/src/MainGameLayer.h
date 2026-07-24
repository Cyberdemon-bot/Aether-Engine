#pragma once
#include <Aether.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <map>
#include <utility>
#include <queue>
#include "Aether/Physics/PhysicsSystem.h"

// --- FLOW FIELD ---
struct FlowCell {
    int       cost      = 1;
    int       bestCost  = 999999;
    glm::vec3 direction = glm::vec3(0.0f);
};

class MainGameLayer : public Aether::Layer
{
public:
    MainGameLayer();
    virtual ~MainGameLayer() = default;

    virtual void Attach()                      override;
    virtual void Detach()                      override;
    virtual void Update(Aether::Timestep ts)   override;
    virtual void OnImGuiRender()               override;
    virtual void OnEvent(Aether::Event& event) override;

private:
    void UpdateMapChunks(const glm::vec3& playerPos);
    void DrawRadar();
    bool WorldToScreen(const glm::vec3& worldPos, const glm::mat4& viewProj, ImVec2 displaySize, ImVec2& outScreen);

private:
    // --- Core ---
    Aether::Scene               m_Scene;
    Aether::EditorCamera        m_Camera;
    Aether::Ref<Aether::Shader> m_MainShader;
    Aether::Importer* m_Importer;

    Aether::Ref<Aether::FrameBuffer> m_MainFbo;
    std::vector<Aether::RenderPass>  m_Pipeline;
    Aether::Entity m_SunLight = Aether::Null_Entity;

    bool m_ShowFlowFieldDebug = false;

    // --- Player ---
    Aether::Entity m_Player         = Aether::Null_Entity;
    float          m_PlayerSpeed    = 10.0f;
    bool           m_IsPlayerMoving = false;
    Aether::Handle<Aether::RigidBody>   m_PlayerBodyHandle{};

    float m_bobSpeed    = 6.0f;
    float m_bobStrength = 0.1f;

    float yFloor = -7.6f;

    float m_PlayerHealth   = 100.0f;
    float m_MaxHealth      = 100.0f;
    float m_DamageCooldown = 1.0f;

    // --- Zombies ---

    Aether::RegisteredScene                m_ZombieSceneData;
    std::vector<Aether::Entity>            m_ActiveZombies;
    float          m_ZombieSpeed = 4.5f;
    Aether::Entity SpawnZombie(const glm::vec3& position);

    int maxZombies = 1000;

    // --- Flow Field ---
    std::map<std::pair<int, int>, FlowCell> m_FlowField;
    float m_PathGridSize      = 1.0f;
    int   m_FlowFieldSubdivisions = 16;
    float m_FlowFieldTimer    = 0.0f;
    void UpdateFlowField(const glm::vec3& targetPos);

    float GetCellValue(int coordX, int coordZ) const;
    int   GetObstacleCost(int coordX, int coordZ) const;
    bool  IsObstacle(const glm::vec3& worldPos) const;
    bool  IsObstacleWithRadius(const glm::vec3& worldPos) const;
    float GetSpeedMultiplier(const glm::vec3& worldPos) const;

    // --- Gun ---
    Aether::Entity m_Gun = Aether::Null_Entity;

    glm::vec3 m_GunPosTP   = { -0.25f,  1.37f, -0.45f };
    glm::vec3 m_GunRotTP   = {  0.0f,  -90.0f,  0.0f  };
    glm::vec3 m_GunScaleTP = {  0.2f,    0.2f,   0.2f  };

    // --- Ammo ---
    int   m_CurrentAmmo    = 30;
    int   m_MaxAmmo        = 30;
    bool  m_IsReloading    = false;
    float m_ReloadTimer    = 0.0f;
    float m_ReloadDuration = 2.5f;
    float m_ReloadRotation = 0.0f;
    float m_AmmoEmptyTimer = 0.0f;

    // --- Dynamic Map ---
    float m_ChunkSize             = 16.0f;
    int   m_BaseRenderDistance    = 5;
    float m_ZoomInfluence         = 5.0f;
    int   m_CurrentRenderDistance = 5;

    struct ChunkData {
        Aether::Entity              landEntity = Aether::Null_Entity;
        std::vector<Aether::Entity> zombies;
        int                         rotation = 0;
    };
    std::map<std::pair<int, int>, ChunkData> m_ActiveChunks;

    Aether::Handle<Aether::Asset>              m_BaseMapMesh;

    // --- Rendering ---
    float m_ShadowBias = 0.00001f;
    bool  m_LockCamera = false;

    std::shared_ptr<Aether::Texture2D> m_MuzzleFlashTexture;
    glm::vec3 m_MuzzleOffset = { 0.0f, -0.25f, 1.2f };

    // --- Fog ---
    int       m_FogMode    = 2;
    glm::vec3 m_FogColor   = glm::vec3(0.5f, 0.6f, 0.7f);
    float     m_FogDensity = 0.03f;
    float     m_FogStart   = 10.0f;
    float     m_FogEnd     = 80.0f;

    Aether::Handle<Aether::AudioSource> m_BgmSource;
    Aether::Handle<Aether::AudioSource> m_GunSource;
    Aether::Handle<Aether::AudioSource> m_ReloadSource;
    Aether::Handle<Aether::AudioSource> m_BiteSource;

    Aether::Prefab m_ZombiePrefab;
    Aether::Prefab m_ChunkPrefab;
    Aether::RegisteredScene m_UploadMap;
    Aether::Handle<Aether::Asset> m_SheetHandle;

    float m_ShootTimer    = 0.0f;
    float m_ShootDuration = 0.3f;

    static constexpr int   k_ObstacleMapSize = 16;
    static constexpr float k_CapsuleRadius   = 0.35f;
    static constexpr float k_CollisionSkin   = 0.15f;
    float m_ObstacleMap[k_ObstacleMapSize][k_ObstacleMapSize] = {
        {0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0},
        {0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0},
        {0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0.5f, 0.5f, 0.5f, 0.5f, 0,    0},
        {0,    0,    0.5f, 0.5f, 0.5f, 0.5f, 0,    0,    0,    0,    0.5f, 1,    1,    0.5f, 0,    0},
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0.5f, 1,    1,    0.5f, 0,    0},
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0.5f, 1,    1,    0.5f, 0,    0},
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0.5f, 1,    1,    0.5f, 0,    0},
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0.5f, 1,    1,    0.5f, 0,    0},
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0.5f, 0.5f, 0.5f, 0.5f, 0,    0},
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0},
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0},
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0},
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0},
        {0,    0,    0.5f, 0.5f, 0.5f, 0.5f, 0,    0,    0,    0,    0.5f, 1,    1,    1,    0.5f, 0},
        {0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0.5f, 1,    1,    1,    0.5f, 0},
        {0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0},
    };
};