#pragma once
#include <Aether.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <map>
#include <utility>
#include <queue>
#include "Aether/Physics/PhysicsSystem.h"
#include <future>
#include <mutex>

// --- HỆ THỐNG FLOW FIELD ---
struct FlowCell {
    int cost = 1;               
    int bestCost = 999999;      
    glm::vec3 direction = glm::vec3(0.0f); 
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
    void DestroyHierarchy(Aether::Entity entity);

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
    std::vector<Aether::RenderPass> m_Pipeline;

    Aether::Entity m_SunLight      = Aether::Null_Entity;
    Aether::Entity m_SelectedEntity = Aether::Null_Entity;

    // --- Nhân Vật (Player) ---
    Aether::Entity m_Player = Aether::Null_Entity;
    Aether::UUID m_RunAnimation  = 0;
    Aether::UUID m_IdleAnimation = 0;
    float m_PlayerSpeed = 10.0f;
    glm::vec3 m_PlayerVelocity = glm::vec3(0.0f);
    Aether::UUID m_PlayerBodyID = 0; // Physics body
    // --- Logic Bobbing ---
    float m_bobSpeed = 12.0f;    // Tốc độ nhịp bước
    float m_bobStrength = 0.08f; // Độ mạnh của cú nảy

    // --- HỆ THỐNG ZOMBIE ---
    Aether::RegisteredScene m_ZombieSceneData;
    std::vector<Aether::Entity> m_ActiveZombies;
    std::map<Aether::Entity, Aether::UUID> m_ZombieAnimators;
    std::map<Aether::Entity, Aether::UUID> m_ZombieBodyIDs; // Physics body per zombie
    Aether::UUID m_ZombieRunAnimation  = 0;
    Aether::UUID m_ZombieIdleAnimation = 0;
    float m_ZombieSpeed = 3.5f;
    Aether::Entity SpawnZombie(const glm::vec3& position);

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
    glm::vec3 m_GunPosTP   = { -0.25f, 1.37f, -0.45f };
    glm::vec3 m_GunRotTP   = { 0.0f, -90.0f, 0.0f };
    glm::vec3 m_GunScaleTP = { 0.2f, 0.2f, 0.2f };

    // --- Logic đạn dược ---
    int   m_CurrentAmmo = 30;           // Đạn trong băng hiện tại
    int   m_MaxAmmo = 30;               // Băng đạn tối đa
    bool  m_IsReloading = false;        // Trạng thái đang nạp đạn
    float m_ReloadTimer = 0.0f;         // Bộ đếm thời gian nạp
    float m_ReloadDuration = 2.5f;      // Thời gian nạp đạn (2.5 giây)

    // --- Hiệu ứng UI ---
    float m_ReloadRotation = 0.0f;      // Góc xoay của tâm hình tròn khi reload

    // --- Hệ thống Map Động theo Zoom ---
    float m_ChunkSize = 2.0f;
    int   m_BaseRenderDistance   = 15;
    float m_ZoomInfluence        = 5.0f;
    int   m_CurrentRenderDistance = 15;

    // PHẢI ĐỊNH NGHĨA STRUCT TRƯỚC KHI DÙNG TRONG MAP
    struct ChunkData {
        Aether::Entity landEntity = Aether::Null_Entity;
        std::vector<Aether::Entity> zombies; 
    };

    // Sau đó mới khai báo map sử dụng ChunkData
    std::map<std::pair<int, int>, ChunkData> m_ActiveChunks;
    
    // Logic tối ưu hóa việc load map
    glm::vec3 m_LastChunkUpdatePos = { 1000.0f, 1000.0f, 1000.0f }; // Vị trí lần cuối cập nhật map
    float m_ChunkUpdateThreshold = 5.0f;

    Aether::Ref<Aether::Mesh>     m_BaseMapMesh;
    Aether::Ref<Aether::Material> m_BaseMapMaterial;
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

    // Luồng cho Flow Field
    std::future<void> m_FlowFieldFuture;
    std::mutex m_FlowFieldMutex;

    // Luồng cho Load Map Chunk
    std::future<void> m_MapChunkFuture;
    std::mutex m_MapChunkMutex;
    uint32_t m_FrameCounter = 0;

    std::shared_ptr<Aether::Texture2D> m_MuzzleFlashTexture; // Lưu ảnh tiadan.png
    glm::vec3 m_MuzzleOffset = { 0.0f, -0.25f, 1.2f };

    int   m_FogMode = 2;                          // 2 = exponential
    glm::vec3 m_FogColor = glm::vec3(0.5f, 0.6f, 0.7f); // Grey-blue mist
    float m_FogDensity = 0.03f;
    float m_FogStart = 10.0f;
    float m_FogEnd = 80.0f;
};  