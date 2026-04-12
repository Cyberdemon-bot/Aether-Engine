#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Renderer/Resource.h"
#include "Aether/Renderer/Texture.h"
#include "Aether/Renderer/Shader.h"
#include "Aether/Core/UUID.h"

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

        void AddTexture(const std::string& name, ResourceHandle handle) { m_Textures.push_back({name, handle}); }
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

        static Ref<Material> Create() { return CreateRef<Material>(); }

    private:

        std::vector<std::pair<std::string, glm::mat4>> m_Mat4Uniforms;
        std::vector<std::pair<std::string, glm::vec4>> m_Vec4Uniforms;
        std::vector<std::pair<std::string, glm::vec3>> m_Vec3Uniforms;
        std::vector<std::pair<std::string, std::vector<int>>> m_IntArrayUniforms;
        std::vector<std::pair<std::string, ResourceHandle>> m_Textures;
        std::vector<std::pair<std::string, int>> m_IntUniforms;
        std::vector<std::pair<std::string, float>> m_FloatUniforms;

        uint32_t m_Flags = (uint32_t)MaterialFlag::None;

        static Scope<Material> CreateImpl() { return CreateScope<Material>(); }
        friend class AssetManager;
    };

    struct AETHER_API MaterialTable
    {
        std::vector<Material*> CachedPtr;
        std::vector<AssetHandle> BaseHandles;
        std::vector<AssetHandle> OverrideHandles;

        void Resize(uint32_t size)
        {
            CachedPtr.resize(size);
            BaseHandles.resize(size);
            OverrideHandles.resize(size);
        }

        void CopyDefaultList(const std::vector<AssetHandle>& handleList);
        void CopyOverrideList(const std::vector<AssetHandle>& handleList);
        void MoveDefaultList(std::vector<AssetHandle>&& handleList);
        void MoveOverrideList(std::vector<AssetHandle>&& handleList);

        void Reset();
        void SetOverride(uint32_t index, AssetHandle handle);
        void SetDefault(uint32_t index, AssetHandle handle);
        void Revert(uint32_t index);
        void SwitchOverride(uint32_t index);
    };
}