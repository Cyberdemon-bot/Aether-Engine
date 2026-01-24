#pragma once

#include <Aether.h>
#include <vector>
#include <glm/glm.hpp>

struct SubMeshInstance
{
    Aether::SubMesh Data;
    
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 Rotation = glm::vec3(0.0f);
    glm::vec3 Scale = glm::vec3(1.0f);
    
    bool Visible = true;
};

struct ModelFile
{
    std::string Name;
    std::string FilePath;
    Aether::Ref<Aether::Mesh> Mesh;
    std::vector<SubMeshInstance> SubMeshes;
    
    glm::vec3 BoundsMin = glm::vec3(0.0f);
    glm::vec3 BoundsMax = glm::vec3(0.0f);
    
    bool IsLoaded = false;
};

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
    void LoadModelFile(const std::string& filepath);
    void LookAtModel();
    void RenderSubMesh(ModelFile& model, SubMeshInstance& submesh);
    
    // Helper function to create and register material from texture data
    Aether::UUID CreateMaterialFromTexture(const unsigned char* data, size_t size, int width, int height, const std::string& name);

private:
    Aether::Ref<Aether::UniformBuffer> m_CameraUBO;
    Aether::EditorCamera m_EditorCamera;
    Aether::UUID m_ShaderId;
    Aether::UUID m_DefaultMaterialId;

    ModelFile m_Model;
    char m_ModelPathBuffer[256] = "assets/models/thanggay.glb"; // Default path
    int m_SelectedSubMeshIndex = -1;
};