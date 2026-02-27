#pragma once
#include <Aether.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <map>
#include <utility>

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
    
    Aether::Entity m_SunLight = Aether::Null_Entity;
    Aether::Entity m_SelectedEntity = Aether::Null_Entity; 

    // --- Nhân Vật (Player) ---
    Aether::Entity m_Player = Aether::Null_Entity;
    Aether::UUID m_RunAnimation = 0;
    Aether::UUID m_IdleAnimation = 0;
    float m_PlayerSpeed = 10.0f; // Tốc độ chạy của nhân vật

    Aether::Entity m_zombie = Aether::Null_Entity;
    Aether::UUID m_ZombieRunAnimation = 0;
    Aether::UUID m_ZombieIdleAnimation = 0;
    float m_ZombieSpeed = 10.0f;

    Aether::Entity m_Gun = Aether::Null_Entity;

    // --- Thông số cấu hình Súng cho Góc nhìn thứ 1 ---
    glm::vec3 m_GunPosFP   = { 0.38f, -0.25f, 1.2f }; // X: Sang phải, Y: Lên xuống, Z: Tiến lùi
    glm::vec3 m_GunRotFP   = { 0.0f, 90.0f, 0.0f }; // Tính bằng Độ (Degrees)
    glm::vec3 m_GunScaleFP = { 0.2f, 0.2f, 0.2f };

    // --- Thông số cấu hình Súng cho Góc nhìn thứ 3 ---
    glm::vec3 m_GunPosTP   = { -0.27f, 1.51f, -0.53f };
    glm::vec3 m_GunRotTP   = { 0.0f, -90.0f, 0.0f };
    glm::vec3 m_GunScaleTP = { 0.2f, 0.2f, 0.2f };

    // --- Hệ thống Map Động theo Zoom ---
    float m_ChunkSize = 2.0f;           // Kích thước 1 ô đất
    int m_BaseRenderDistance = 15;        // Bán kính tối thiểu (khi zoom sát người)
    float m_ZoomInfluence = 5.0f;       // Cứ xa ra thêm 15 đơn vị camera, đẻ thêm 1 vòng map
    int m_CurrentRenderDistance = 15;     // Biến nội bộ để lưu bán kính thực tế

    Aether::Ref<Aether::Mesh> m_BaseMapMesh; // Mesh gốc của Map
    Aether::Ref<Aether::Material> m_BaseMapMaterial;
    std::map<std::pair<int, int>, Aether::Entity> m_ActiveChunks;
    
    std::vector<Aether::UUID> m_LoadedMeshes; 

    // --- Biến cho UI & Đồ họa ---
    bool  m_AutoRotate    = false;
    float m_RotationSpeed = 1.0f;

    uint32_t m_LightIdx = 0;
    float m_VolDensity   = 0.02f;
    float m_VolIntensity = 1.0f;
    int   m_VolSteps     = 64;
    float m_ShadowBias   = 0.00001f;
    bool m_LockCamera = false;
    bool m_FirstPerson = false; // Mặc định là góc nhìn thứ 3

    // --- Quản lý Animation Súng ---
    Aether::UUID m_ShootAnimation = 0; // Lưu ID của clip bắn súng
    bool m_IsShooting = false;     // Trạng thái khóa (cooldown)
    float m_ShootTimer = 0.0f;     // Bộ đếm thời gian
    float m_FireRate = 0.5f;       // Thời gian của 1 nhịp bắn (Ví dụ 0.5 giây, hãy tự chỉnh cho khớp với độ dài file .glb)
};
