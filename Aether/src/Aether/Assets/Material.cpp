#include "aepch.h"
#include "Aether/Core/Assert.h"
#include "Aether/Renderer/ResourceManager.h"
#include "Aether/Assets/Material.h"
#include "Aether/Renderer/Texture.h"

namespace Aether {

    void AMaterial::AddImage(std::string_view name, Handle<Asset> handle) 
    { 
        m_Images.push_back({std::string(name), handle}); 
    }

    void AMaterial::AddFloat(std::string_view name, float value) 
    { 
        m_FloatUniforms.push_back({std::string(name), value}); 
    }

    void AMaterial::AddInt(std::string_view name, int value) 
    { 
        m_IntUniforms.push_back({std::string(name), value}); 
    }

    void AMaterial::AddIntArray(std::string_view name, int* values, uint32_t count)
    {
        auto it = std::find_if(m_IntArrayUniforms.begin(), m_IntArrayUniforms.end(), 
            [&name](const auto& pair) 
            { 
                return pair.first == name; 
            });
            
        if (it != m_IntArrayUniforms.end()) 
            it->second.assign(values, values + count);
    }

    void AMaterial::AddVec3(std::string_view name, const glm::vec3& value) 
    { 
        m_Vec3Uniforms.push_back({std::string(name), value}); 
    }

    void AMaterial::AddVec4(std::string_view name, const glm::vec4& value) 
    { 
        m_Vec4Uniforms.push_back({std::string(name), value}); 
    }

    void AMaterial::AddMat4(std::string_view name, const glm::mat4& value) 
    { 
        m_Mat4Uniforms.push_back({std::string(name), value}); 
    }

    void AMaterial::AddFlag(MaterialFlag flag) 
    { 
        m_Flags |= (uint32_t)flag; 
    }

    void AMaterial::RemoveFlag(MaterialFlag flag) 
    { 
        m_Flags &= ~(uint32_t)flag; 
    }

    void AMaterial::ToggleFlag(MaterialFlag flag) 
    { 
        m_Flags ^= (uint32_t)flag; 
    }

    bool AMaterial::HasFlag(MaterialFlag flag) const 
    { 
        return m_Flags & (uint32_t)flag; 
    }

    void ASheet::CopyDefaultList(const std::vector<Handle<Asset>>& handleList)
    {
        BaseHandles = handleList;
        OverrideHandles.resize(BaseHandles.size(), Handle<Asset>::MakeInvalid());
    }

    void ASheet::CopyOverrideList(const std::vector<Handle<Asset>>& handleList)
    {
        OverrideHandles = handleList;
    }

    void ASheet::MoveDefaultList(std::vector<Handle<Asset>>&& handleList)
    {
        BaseHandles = std::move(handleList);
        OverrideHandles.resize(BaseHandles.size(), Handle<Asset>::MakeInvalid());
    }

    void ASheet::MoveOverrideList(std::vector<Handle<Asset>>&& handleList)
    {
        OverrideHandles = std::move(handleList);
    }

    void ASheet::Reset()
    {
        std::fill(OverrideHandles.begin(), OverrideHandles.end(), Handle<Asset>::MakeInvalid());
    }

    void ASheet::SetOverride(uint32_t index, Handle<Asset> handle)
    {
        AE_CORE_ASSERT(index < OverrideHandles.size(), "Index out of bounds in Sheet!");
        OverrideHandles[index] = handle;
    }

    void ASheet::SetDefault(uint32_t index, Handle<Asset> handle)
    {
        AE_CORE_ASSERT(index < BaseHandles.size(), "Index out of bounds in Sheet!");
        BaseHandles[index] = handle;
    }

    void ASheet::Revert(uint32_t index)
    {
        AE_CORE_ASSERT(index < OverrideHandles.size(), "Index out of bounds in Sheet!");
        OverrideHandles[index] = Handle<Asset>::MakeInvalid();
    }

    Handle<Asset> ASheet::GetActiveHandle(uint32_t index) const
    {
        if (OverrideHandles[index].IsValid())
            return OverrideHandles[index];
            
        return BaseHandles[index];
    }

    uint32_t ASheet::GetSize()
    {
        return BaseHandles.size();
    }
}