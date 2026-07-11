#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"
#include "Aether/Renderer/Resource.h"
#include "Aether/Renderer/Shader.h"

#include <string>
#include <vector>
#include <algorithm>

namespace Aether {
    enum class MaterialFlag
    {
        None = 0,
        DepthTest = BIT(1),
        Blend = BIT(2),
        TwoSided = BIT(3),
        DisableShadowCasting = BIT(4),
        DisableShadowReceiving = BIT(5)
    };

    class AETHER_API Material : public Asset
    {
    public:
        Material() {};

        void UploadMaterial(Shader* shader, uint32_t startSlot = 0);

        void AddTexture(const std::string& name, Handle<Resource> handle) { m_Textures.push_back({name, handle}); }
        void AddFloat(const std::string& name, float value) { m_FloatUniforms.push_back({name, value}); }
        void AddInt(const std::string& name, int value) { m_IntUniforms.push_back({name, value}); }
        void AddIntArray(const std::string& name, int* values, uint32_t count)
        {
            auto it = std::find_if(m_IntArrayUniforms.begin(), m_IntArrayUniforms.end(), 
                [name](const auto& pair) { return pair.first == name; });
            if (it != m_IntArrayUniforms.end()) it->second.assign(values, values + count);
        }
        void AddVec3(const std::string& name, const glm::vec3& value) { m_Vec3Uniforms.push_back({name, value}); }
        void AddVec4(const std::string& name, const glm::vec4& value) { m_Vec4Uniforms.push_back({name, value}); }
        void AddMat4(const std::string& name, const glm::mat4& value) { m_Mat4Uniforms.push_back({name, value}); }

        void AddFlag(MaterialFlag flag) { m_Flags |= (uint32_t)flag; }
        void RemoveFlag(MaterialFlag flag) { m_Flags &= ~(uint32_t)flag; }
        void ToggleFlag(MaterialFlag flag) { m_Flags ^= (uint32_t)flag; }
        bool HasFlag(MaterialFlag flag) const { return m_Flags & (uint32_t)flag; }

    private:

        std::vector<std::pair<std::string, glm::mat4>> m_Mat4Uniforms;
        std::vector<std::pair<std::string, glm::vec4>> m_Vec4Uniforms;
        std::vector<std::pair<std::string, glm::vec3>> m_Vec3Uniforms;
        std::vector<std::pair<std::string, std::vector<int>>> m_IntArrayUniforms;
        std::vector<std::pair<std::string, Handle<Resource>>> m_Textures;
        std::vector<std::pair<std::string, int>> m_IntUniforms;
        std::vector<std::pair<std::string, float>> m_FloatUniforms;

        uint32_t m_Flags = (uint32_t)MaterialFlag::None;
    };

    struct AETHER_API Sheet : public Asset
    {
        std::vector<Handle<Asset>> BaseHandles;
        std::vector<Handle<Asset>> OverrideHandles;

        void Resize(uint32_t size)
        {
            BaseHandles.resize(size, Handle<Asset>::MakeInvalid());
            OverrideHandles.resize(size, Handle<Asset>::MakeInvalid());
        }

        void CopyDefaultList(const std::vector<Handle<Asset>>& handleList);
        void CopyOverrideList(const std::vector<Handle<Asset>>& handleList);
        void MoveDefaultList(std::vector<Handle<Asset>>&& handleList);
        void MoveOverrideList(std::vector<Handle<Asset>>&& handleList);

        void Reset();
        void SetOverride(uint32_t index, Handle<Asset> handle);
        void SetDefault(uint32_t index, Handle<Asset> handle);
        void Revert(uint32_t index);
        
        Handle<Asset> GetActiveHandle(uint32_t index) const
        {
            if (OverrideHandles[index].IsValid())
                return OverrideHandles[index];
                
            return BaseHandles[index];
        }

        uint32_t GetSize()
        {
            return BaseHandles.size();
        }
    };
}