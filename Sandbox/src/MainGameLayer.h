#pragma once
#include <Aether.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <map>
#include <utility>
#include <queue>

// --- HỆ THỐNG FLOW FIELD ---
struct FlowCell {
    int cost = 1;               // 1 = Đi được, 255 = Chướng ngại vật (Tường)
    int bestCost = 999999;      // Khoảng cách ngắn nhất tới Player
    glm::vec3 direction = glm::vec3(0.0f); // Hướng đi lý tưởng tại ô này
};

class MainGameLayer : public Aether::Layer
{
public:
    MainGameLayer();
    virtual ~MainGameLayer() = default;

    virtual void Attach()                          override;
    virtual void Detach()                          override;
    virtual void Update(Aether::Timestep ts)       override;
    virtual void OnImGuiRender()                   override;
    virtual void OnEvent(Aether::Event& event)     override;

private:
    void UpdateMapChunks(const glm::vec3& playerPos);

    // --- Các hàm vẽ giao diện ImGui ---
    void DrawHierarchyPanel();
    void DrawEntityNode(Aether::Entity entity);
    void DrawScenePanel();
    void DrawLightingPanel();

private:
    // --- Lõi Game ---
    Aether::Scene m_Scene;
    Aether::EditorCamera m_Camera;
    Aether::Ref<Aether::Shader> m_ShadowShader;
    Aether::Ref<Aether::Shader> m_MainShader;
    Aether::Ref<Aether::Shader> m_VolShader;

    Aether::Entity m_SunLight      = Aether::Null_Entity;
    Aether::Entity m_SelectedEntity = Aether::Null_Entity;

    // --- Nhân Vật (Player) ---
    Aether::Entity m_Player = Aether::Null_Entity;
    Aether::UUID m_RunAnimation  = 0;
    Aether::UUID m_IdleAnimation = 0;
    float m_PlayerSpeed = 10.0f;
    glm::vec3 m_PlayerVelocity = glm::vec3(0.0f);

    // --- HỆ THỐNG ZOMBIE ---
    // Chỉ lưu RegisteredScene (UUID + SceneHierarchy), không có GPU data nặng
    Aether::RegisteredScene m_ZombieSceneData;
    std::vector<Aether::Entity> m_ActiveZombies;
    std::map<Aether::Entity, Aether::UUID> m_ZombieAnimators; // Mỗi zombie có animator riêng
    Aether::UUID m_ZombieRunAnimation  = 0;
    Aether::UUID m_ZombieIdleAnimation = 0;
    float m_ZombieSpeed = 3.5f;
    void SpawnZombie(const glm::vec3& position);

    // --- HỆ THỐNG FLOW FIELD ---
    std::map<std::pair<int, int>, FlowCell> m_FlowField;
    float m_PathGridSize  = 1.0f;
    float m_FlowFieldTimer = 0.0f;
    void UpdateFlowField(const glm::vec3& targetPos);

    // --- Súng ---
    Aether::Entity m_Gun = Aether::Null_Entity;

    // --- Thông số cấu hình Súng cho Góc nhìn thứ 1 ---
    glm::vec3 m_GunPosFP   = { 0.38f, -0.25f, 1.2f };
    glm::vec3 m_GunRotFP   = { 0.0f, 90.0f, 0.0f };
    glm::vec3 m_GunScaleFP = { 0.2f, 0.2f, 0.2f };

    // --- Thông số cấu hình Súng cho Góc nhìn thứ 3 ---
    glm::vec3 m_GunPosTP   = { -0.27f, 1.51f, -0.53f };
    glm::vec3 m_GunRotTP   = { 0.0f, -90.0f, 0.0f };
    glm::vec3 m_GunScaleTP = { 0.2f, 0.2f, 0.2f };

    // --- Hệ thống Map Động theo Zoom ---
    float m_ChunkSize = 2.0f;
    int   m_BaseRenderDistance   = 15;
    float m_ZoomInfluence        = 5.0f;
    int   m_CurrentRenderDistance = 15;

    Aether::Ref<Aether::Mesh>     m_BaseMapMesh;
    Aether::Ref<Aether::Material> m_BaseMapMaterial;
    std::map<std::pair<int, int>, Aether::Entity> m_ActiveChunks;
    std::vector<Aether::UUID> m_LoadedMeshes;

    // --- Biến cho UI & Đồ họa ---
    bool  m_AutoRotate    = false;
    float m_RotationSpeed = 1.0f;

    uint32_t m_LightIdx   = 0;
    float m_VolDensity    = 0.02f;
    float m_VolIntensity  = 1.0f;
    int   m_VolSteps      = 64;
    float m_ShadowBias    = 0.00001f;
    bool  m_LockCamera    = false;
    bool  m_FirstPerson   = false;

    // --- Quản lý Animation Súng ---
    Aether::UUID m_ShootAnimation = 0;
    bool  m_IsShooting  = false;
    float m_ShootTimer  = 0.0f;
    float m_FireRate    = 0.5f;
};