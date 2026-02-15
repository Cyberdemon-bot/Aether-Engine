#include "Aether/Resources/Material.h"

namespace Aether {

    void Material::UploadMaterial(Ref<Shader> shader, uint32_t startSlot)
    {
        shader->Bind();
        for (const auto& [name, fval] : m_FloatUniforms) shader->SetFloat(name, fval);
        for (const auto& [name, ival] : m_IntUniforms) shader->SetInt(name, ival);
        for (const auto& [name, ivec] : m_IntArrayUniforms) shader->SetIntArray(name, ivec.data(), (uint32_t)ivec.size());
        for (const auto& [name, vec3] : m_Vec3Uniforms) shader->SetFloat3(name, vec3);
        for (const auto& [name, vec4] : m_Vec4Uniforms) shader->SetFloat4(name, vec4);
        for (const auto& [name, mat4] : m_Mat4Uniforms) shader->SetMat4(name, mat4);
        for (const auto& [name, texture] : m_Textures)
        {
            texture->Bind(startSlot);
            shader->SetInt(name, startSlot);
            startSlot++;
        }
    }

    void MaterialLibrary::Init()
    {
        GetMaterials().reserve(128);
        AE_CORE_INFO("MaterialLibrary initialized");
    }

    void MaterialLibrary::Shutdown()
    {
        GetMaterials().clear();
    }

    void MaterialLibrary::Add(Ref<Material> obj, UUID id)
    {
        auto& materials = GetMaterials();
        if (materials.find(id) != materials.end())
        {
            AE_CORE_ERROR("Material Library: ID already exists");
            return;
        }

        if (!obj)
        {
            AE_CORE_ERROR("Material Library: Cannot add null obj");
            return;
        }
        materials[id] = obj;
    }

    Ref<Material> MaterialLibrary::Get(UUID id)
    {
        auto& materials = GetMaterials();
        if (materials.find(id) != materials.end()) 
            return materials[id];

        AE_CORE_WARN("Material Library: ID not found!");
        return nullptr;
    }

    bool MaterialLibrary::Exists(UUID id)
    {
        auto& materials = GetMaterials();
        return materials.find(id) != materials.end();
    }

    std::unordered_map<UUID, Ref<Material>>& MaterialLibrary::GetMaterials()
    {
        static std::unordered_map<UUID, Ref<Material>> s_Materials;
        return s_Materials;
    }
}