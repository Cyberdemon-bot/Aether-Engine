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
    void DrawEntityNode(Entity entity);
    void DrawScenePanel();
    void DrawLightingPanel();

private:
    // --- Lõi Game ---
    Aether::Scene m_Scene;
    Aether::EditorCamera m_Camera; 
    Aether::Ref<Aether::Shader> m_ShadowShader;
    Aether::Ref<Aether::Shader> m_MainShader;
    Aether::Ref<Aether::Shader> m_VolShader; 
    
    Entity m_SunLight = Null_Entity;
    Entity m_SelectedEntity = Null_Entity; 

    // --- Nhân Vật (Player) ---
    Entity m_Player = Null_Entity;
    Aether::UUID m_RunAnimation = 0;
    Aether::UUID m_IdleAnimation = 0;
    float m_PlayerSpeed = 10.0f; // Tốc độ chạy của nhân vật

    // --- Hệ thống Map Động theo Zoom ---
    float m_ChunkSize = 2.0f;           // Kích thước 1 ô đất
    int m_BaseRenderDistance = 15;        // Bán kính tối thiểu (khi zoom sát người)
    float m_ZoomInfluence = 5.0f;       // Cứ xa ra thêm 15 đơn vị camera, đẻ thêm 1 vòng map
    int m_CurrentRenderDistance = 15;     // Biến nội bộ để lưu bán kính thực tế

    Aether::UUID m_BaseMapMesh; // Mesh gốc của Map
    std::map<std::pair<int, int>, Entity> m_ActiveChunks;
    
    std::vector<Aether::UUID> m_LoadedMeshes; 

    // --- Biến cho UI & Đồ họa ---
    bool  m_AutoRotate    = false;
    float m_RotationSpeed = 1.0f;

    uint32_t m_LightIdx = 0;
    float m_VolDensity   = 0.02f;
    float m_VolIntensity = 1.0f;
    int   m_VolSteps     = 64;
    float m_ShadowBias   = 0.00001f;
};
