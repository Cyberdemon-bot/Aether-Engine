#include "aepch.h"
#include "Aether/Renderer/ResourceManager.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Assets/Material.h"

namespace Aether {

    void Material::UploadMaterial(Shader* shader, uint32_t startSlot)
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
            ResourceManager::GetResource<Texture2D>(texture)->Bind(startSlot);
            shader->SetInt(name, startSlot);
            startSlot++;
        }
    }

    void MaterialTable::Reset()
    {
        for(size_t i = 0; i < BaseHandles.size(); i++)
            CachedPtr[i] = AssetManager::GetAsset<Material>(BaseHandles[i]);
    }

    void MaterialTable::SetDefault(uint32_t index, AssetHandle handle)
    {
        if (index >= BaseHandles.size())
        {
            AE_CORE_ERROR("Index out of bounds in MaterialTable");
            return;
        }
        BaseHandles[index] = handle;
        CachedPtr[index] = AssetManager::GetAsset<Material>(BaseHandles[index]);
    }

    void MaterialTable::SetOverride(uint32_t index, AssetHandle handle)
    {
        if (index >= OverrideHandles.size())
        {
            AE_CORE_ERROR("Index out of bounds in MaterialTable");
            return;
        }
        OverrideHandles[index] = handle;
        CachedPtr[index] = AssetManager::GetAsset<Material>(OverrideHandles[index]);
    }

    void MaterialTable::Revert(uint32_t index)
    {
        if (index >= BaseHandles.size())
        {
            AE_CORE_ERROR("Index out of bounds in MaterialTable");
            return;
        }
        CachedPtr[index] = AssetManager::GetAsset<Material>(BaseHandles[index]);
    }

    void MaterialTable::SwitchOverride(uint32_t index)
    {
        if (index >= OverrideHandles.size())
        {
            AE_CORE_ERROR("Index out of bounds in MaterialTable");
            return;
        }
        CachedPtr[index] = AssetManager::GetAsset<Material>(OverrideHandles[index]);
    }

    void MaterialTable::CopyDefaultList(const std::vector<AssetHandle>& handleList)
    {
        Resize((uint32_t)handleList.size()); 
        BaseHandles = handleList;           
    }

    void MaterialTable::MoveDefaultList(std::vector<AssetHandle>&& handleList)
    {
        uint32_t newSize = (uint32_t)handleList.size();
        BaseHandles = std::move(handleList); 
        CachedPtr.resize(newSize);
        OverrideHandles.resize(newSize);
    }

    void MaterialTable::CopyOverrideList(const std::vector<AssetHandle>& handleList)
    {
        uint32_t targetSize = (uint32_t)BaseHandles.size();
        OverrideHandles.resize(targetSize); 
        uint32_t copyCount = std::min((uint32_t)handleList.size(), targetSize);
        std::copy(handleList.begin(), handleList.begin() + copyCount, OverrideHandles.begin());
        for (uint32_t i = copyCount; i < targetSize; ++i) OverrideHandles[i].MakeInvalid(); 
    }

    void MaterialTable::MoveOverrideList(std::vector<AssetHandle>&& handleList)
    {
        uint32_t targetSize = (uint32_t)BaseHandles.size();

        if (handleList.size() <= targetSize)
        {
            OverrideHandles = std::move(handleList);
            OverrideHandles.resize(targetSize); 
        }
        else
        {
            OverrideHandles.resize(targetSize);
            std::move(handleList.begin(), handleList.begin() + targetSize, OverrideHandles.begin());
        }
    }
}