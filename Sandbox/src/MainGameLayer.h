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

    void DrawHierarchyPanel();
    void DrawEntityNode(Aether::Entity entity);
    void DrawScenePanel();
    void DrawLightingPanel();
    bool WorldToScreen(const glm::vec3& worldPos, const glm::mat4& viewProj, ImVec2 displaySize, ImVec2& outScreen);

private:
    // --- Core ---
    Aether::Scene               m_Scene;
    Aether::EditorCamera        m_Camera;
    Aether::Ref<Aether::Shader> m_ShadowShader;
    Aether::Ref<Aether::Shader> m_MainShader;
    std::vector<Aether::RenderPass> m_Pipeline;

    Aether::Entity m_SunLight       = Aether::Null_Entity;
    Aether::Entity m_SelectedEntity = Aether::Null_Entity;

    bool m_ShowFlowFieldDebug = true;

    // --- Player ---
    Aether::Entity m_Player         = Aether::Null_Entity;
    Aether::UUID   m_RunAnimation   = 0;
    float          m_PlayerSpeed    = 10.0f;
    bool           m_IsPlayerMoving = false;
    Aether::UUID   m_PlayerBodyID   = 0;

    float m_bobSpeed    = 6.0f;
    float m_bobStrength = 0.1f;

    float yFloor = -7.6f;

    // --- Zombies ---
    struct ZombieRecord {
        Aether::UUID animatorID = 0;
        Aether::UUID bodyID     = 0;
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

    int GetObstacleCost(int coordX, int coordZ) const;

    // --- Gun ---
    Aether::Entity m_Gun = Aether::Null_Entity;
    Aether::UUID m_GunSound;

    glm::vec3 m_GunPosTP   = { -0.25f,  1.37f, -0.45f };
    glm::vec3 m_GunRotTP   = {  0.0f,  -90.0f,  0.0f  };
    glm::vec3 m_GunScaleTP = {  0.2f,    0.2f,  0.2f  };

    bool  m_IsShooting    = false;
    float m_ShootTimer    = 0.0f;
    float m_ShootDuration = 0.3f;

    // --- Ammo ---
    int   m_CurrentAmmo    = 30;
    int   m_MaxAmmo        = 30;
    bool  m_IsReloading    = false;
    float m_ReloadTimer    = 0.0f;
    float m_ReloadDuration = 2.5f;
    float m_ReloadRotation = 0.0f;

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

    Aether::Ref<Aether::Mesh>                  m_BaseMapMesh;
    std::vector<Aether::Ref<Aether::Material>> m_BaseMapMaterials;

    // --- Rendering ---
    float m_ShadowBias = 0.00001f;
    bool  m_LockCamera = false;

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

    // hardcode matrix
    static constexpr int k_ObstacleMapSize = 16;
    int m_ObstacleMap[k_ObstacleMapSize][k_ObstacleMapSize] = {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0},
        {0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0},
        {0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0},
        {0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0},
        {0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0},
        {0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0},
        {0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0},
        {0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
        {0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    };
};