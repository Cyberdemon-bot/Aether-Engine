#include "aepch.h"
#include "Aether/Core/Assert.h"
#include "Aether/Renderer/ResourceManager.h"
#include "Aether/Assets/Material.h"
#include "Aether/Renderer/Texture.h"

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

    void Sheet::CopyDefaultList(const std::vector<Handle<Asset>>& handleList)
    {
        BaseHandles = handleList;
    }

    void Sheet::CopyOverrideList(const std::vector<Handle<Asset>>& handleList)
    {
        OverrideHandles = handleList;
    }

    void Sheet::MoveDefaultList(std::vector<Handle<Asset>>&& handleList)
    {
        BaseHandles = std::move(handleList);
    }

    void Sheet::MoveOverrideList(std::vector<Handle<Asset>>&& handleList)
    {
        OverrideHandles = std::move(handleList);
    }

    void Sheet::Reset()
    {
        std::fill(OverrideHandles.begin(), OverrideHandles.end(), Handle<Asset>::MakeInvalid());
    }

    void Sheet::SetOverride(uint32_t index, Handle<Asset> handle)
    {
        AE_CORE_ASSERT(index < OverrideHandles.size(), "Index out of bounds in Sheet!");
        OverrideHandles[index] = handle;
    }

    void Sheet::SetDefault(uint32_t index, Handle<Asset> handle)
    {
        AE_CORE_ASSERT(index < BaseHandles.size(), "Index out of bounds in Sheet!");
        BaseHandles[index] = handle;
    }

    void Sheet::Revert(uint32_t index)
    {
        AE_CORE_ASSERT(index < OverrideHandles.size(), "Index out of bounds in Sheet!");
        OverrideHandles[index] = Handle<Asset>::MakeInvalid();
    }
}