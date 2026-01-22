#pragma once

#include <Aether.h>
#include <vector>
#include <glm/glm.hpp>

class ModelLoaderLayer : public Aether::Layer
{
public:
    ModelLoaderLayer();
    virtual ~ModelLoaderLayer() = default;

    virtual void Attach() override;
    virtual void Detach() override;
    virtual void Update(Aether::Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Aether::Event& event) override;

private:
    void LoadModel(const std::string& filepath);
    void LookAtModel();

private:
    // Rendering
    Aether::Ref<Aether::Mesh> m_ModelMesh;
    Aether::Ref<Aether::Material> m_Material;
    Aether::Ref<Aether::UniformBuffer> m_CameraUBO;
    
    Aether::EditorCamera m_EditorCamera;

    // Model data
    std::vector<Aether::SubMesh> m_SubMeshes;
    glm::vec3 m_ModelBoundsMin = glm::vec3(0.0f);
    glm::vec3 m_ModelBoundsMax = glm::vec3(0.0f);

    // Transform
    glm::vec3 m_ModelPosition = glm::vec3(0.0f);
    glm::vec3 m_ModelRotation = glm::vec3(0.0f); // Euler angles in degrees
    glm::vec3 m_ModelScale = glm::vec3(1.0f);

    // UI
    bool m_ModelLoaded = false;
    std::string m_ModelPath = "assets/models/robot.glb";
};