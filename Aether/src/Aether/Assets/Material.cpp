#include "aepch.h"
#include "Aether/Core/Assert.h"
#include "Aether/Renderer/ResourceManager.h"
#include "Aether/Assets/Material.h"
#include "Aether/Renderer/Texture.h"

namespace Aether {

    void Material::AddTexture(const std::string& name, Handle<Resource> handle) 
    { 
        m_Textures.push_back({name, handle}); 
    }

    void Material::AddFloat(const std::string& name, float value) 
    { 
        m_FloatUniforms.push_back({name, value}); 
    }

    void Material::AddInt(const std::string& name, int value) 
    { 
        m_IntUniforms.push_back({name, value}); 
    }

    void Material::AddIntArray(const std::string& name, int* values, uint32_t count)
    {
        auto it = std::find_if(m_IntArrayUniforms.begin(), m_IntArrayUniforms.end(), 
            [&name](const auto& pair) 
            { 
                return pair.first == name; 
            });
            
        if (it != m_IntArrayUniforms.end()) 
            it->second.assign(values, values + count);
    }

    void Material::AddVec3(const std::string& name, const glm::vec3& value) 
    { 
        m_Vec3Uniforms.push_back({name, value}); 
    }

    void Material::AddVec4(const std::string& name, const glm::vec4& value) 
    { 
        m_Vec4Uniforms.push_back({name, value}); 
    }

    void Material::AddMat4(const std::string& name, const glm::mat4& value) 
    { 
        m_Mat4Uniforms.push_back({name, value}); 
    }

    void Material::AddFlag(MaterialFlag flag) 
    { 
        m_Flags |= (uint32_t)flag; 
    }

    void Material::RemoveFlag(MaterialFlag flag) 
    { 
        m_Flags &= ~(uint32_t)flag; 
    }

    void Material::ToggleFlag(MaterialFlag flag) 
    { 
        m_Flags ^= (uint32_t)flag; 
    }

    bool Material::HasFlag(MaterialFlag flag) const 
    { 
        return m_Flags & (uint32_t)flag; 
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