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
    Aether::Ref<Aether::Shader> m_ShadowShader;
    Aether::Ref<Aether::Shader> m_MainShader;
    Aether::Ref<Aether::FrameBuffer> m_ShadowFbo;
    Aether::Ref<Aether::FrameBuffer> m_MainFbo;
    std::vector<Aether::RenderPass> m_Pipeline;

    Aether::Entity m_SunLight       = Aether::Null_Entity;

    bool m_ShowFlowFieldDebug = false;


    // --- Player ---
    Aether::Entity m_Player         = Aether::Null_Entity;
    Aether::UUID   m_RunAnimation   = 0;
    float          m_PlayerSpeed    = 10.0f;
    bool           m_IsPlayerMoving = false;
    Aether::BodyHandle   m_PlayerBodyHandle{};

    float m_bobSpeed    = 6.0f;
    float m_bobStrength = 0.1f;

    float yFloor = -7.6f;

    float m_PlayerHealth = 100.0f;    // Máu hiện tại
    float m_MaxHealth = 100.0f;       // Máu tối đa
    float m_DamageCooldown = 1.0f;    // Thời gian chờ giữa các lần bị cắn (để không chết ngay lập tức)

    // --- Zombies ---
    struct ZombieRecord {
        Aether::UUID animatorID = 0;
        Aether::BodyHandle bodyHandle{};
    };

    Aether::RegisteredScene                    m_ZombieSceneData;
    std::vector<Aether::Entity>                m_ActiveZombies;
    std::map<Aether::Entity, ZombieRecord>     m_ZombieRegistry;
    Aether::UUID  m_ZombieRunAnimation = 0;
    float         m_ZombieSpeed        = 4.5f;
    Aether::Entity SpawnZombie(const glm::vec3& position);

    int maxZombies = 100;
    // --- Flow Field ---
    std::map<std::pair<int, int>, FlowCell> m_FlowField;
    float m_PathGridSize = 1.0f;
    int   m_FlowFieldSubdivisions = 16;
    float m_FlowFieldTimer = 0.0f;
    void UpdateFlowField(const glm::vec3& targetPos);

    float GetCellValue(int coordX, int coordZ) const;
    int   GetObstacleCost(int coordX, int coordZ) const;
    bool  IsObstacle(const glm::vec3& worldPos) const;
    bool  IsObstacleWithRadius(const glm::vec3& worldPos) const; // uses k_CapsuleRadius + k_CollisionSkin
    float GetSpeedMultiplier(const glm::vec3& worldPos) const;


    // --- Gun ---
    Aether::Entity m_Gun = Aether::Null_Entity;

    glm::vec3 m_GunPosFP   = {  0.38f, -0.25f,  1.2f };
    glm::vec3 m_GunRotFP   = {  0.0f,   90.0f,  0.0f };
    glm::vec3 m_GunScaleFP = {  0.2f,    0.2f,  0.2f };

    glm::vec3 m_GunPosTP   = { -0.25f,  1.37f, -0.45f };
    glm::vec3 m_GunRotTP   = {  0.0f,  -90.0f,  0.0f  };
    glm::vec3 m_GunScaleTP = {  0.2f,    0.2f,  0.2f  };

    // --- Ammo ---
    int   m_CurrentAmmo    = 30;
    int   m_MaxAmmo        = 30;
    bool  m_IsReloading    = false;
    float m_ReloadTimer    = 0.0f;
    float m_ReloadDuration = 2.5f;
    float m_ReloadRotation = 0.0f;
    float m_AmmoEmptyTimer = 0.0f;  // Bộ đếm thời gian (tính bằng giây) để chạy hiệu ứng nhảy

    // --- Dynamic Map ---
    float m_ChunkSize             = 16.0f;
    int   m_BaseRenderDistance    = 5;
    float m_ZoomInfluence         = 5.0f;
    int   m_CurrentRenderDistance = 5;

    struct ChunkData {
        Aether::Entity              landEntity = Aether::Null_Entity;
        std::vector<Aether::Entity> zombies;
        int                         rotation = 0; // 0-3 (multiples of 90)
    };  
    std::map<std::pair<int, int>, ChunkData> m_ActiveChunks;

    Aether::AssetHandle m_BaseMapMesh;
    std::vector<Aether::AssetHandle> m_BaseMapMaterials;

    // --- Rendering ---
    float m_ShadowBias  = 0.00001f;
    bool  m_LockCamera  = false;
    bool  m_FirstPerson = false;

    // --- Gun Animation ---
    Aether::UUID m_ShootAnimation = 0;

    std::shared_ptr<Aether::Texture2D> m_MuzzleFlashTexture;
    glm::vec3 m_MuzzleOffset = { 0.0f, -0.25f, 1.2f };

    // --- Fog ---
    int       m_FogMode    = 2;
    glm::vec3 m_FogColor   = glm::vec3(0.5f, 0.6f, 0.7f);
    float     m_FogDensity = 0.03f;
    float     m_FogStart   = 10.0f;
    float     m_FogEnd     = 80.0f;

    Aether::UUID m_GunSoundID;
    Aether::UUID m_GunReloadID;
    Aether::UUID m_ZombieBiteID;
    Aether::UUID m_BgmSoundID;
    std::vector<Aether::UUID> sources;

    float m_ShootTimer    = 0.0f;
    float m_ShootDuration = 0.3f;

    // hardcode matrix — 0: free, 0.5: slow zone (building edge), 1: solid wall
    static constexpr int   k_ObstacleMapSize = 16;
    static constexpr float k_CapsuleRadius   = 0.35f;
    static constexpr float k_CollisionSkin   = 0.15f; // extra margin so block triggers before touching wall
    float m_ObstacleMap[k_ObstacleMapSize][k_ObstacleMapSize] = {
        {0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0},  // row 0
        {0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0},  // row 1
        {0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0.5f, 0.5f, 0.5f, 0.5f, 0,    0},  // row 2
        {0,    0,    0.5f, 0.5f, 0.5f, 0.5f, 0,    0,    0,    0,    0.5f, 1,    1,    0.5f, 0,    0},  // row 3
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0.5f, 1,    1,    0.5f, 0,    0},  // row 4
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0.5f, 1,    1,    0.5f, 0,    0},  // row 5
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0.5f, 1,    1,    0.5f, 0,    0},  // row 6
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0.5f, 1,    1,    0.5f, 0,    0},  // row 7
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0.5f, 0.5f, 0.5f, 0.5f, 0,    0},  // row 8
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0},  // row 9
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0},  // row 10
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0},  // row 11
        {0,    0,    0.5f, 1,    1,    0.5f, 0,    0,    0,    0,    0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0},  // row 12
        {0,    0,    0.5f, 0.5f, 0.5f, 0.5f, 0,    0,    0,    0,    0.5f, 1,    1,    1,    0.5f, 0},  // row 13
        {0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0.5f, 1,    1,    1,    0.5f, 0},  // row 14
        {0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0},  // row 15
    };
};