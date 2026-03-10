#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Renderer/Resource.h"
#include "Aether/Renderer/Texture.h"
#include "Aether/Renderer/Shader.h"
#include "Aether/Core/UUID.h"

#include <string>
#include <unordered_map>

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

        void AddTexture(const std::string& name, ResourceHandle handle) { m_Textures[name] = handle; }
        void AddFloat(const std::string& name, float value) { m_FloatUniforms[name] = value; }
        void AddInt(const std::string& name, int value) { m_IntUniforms[name] = value; }
        void AddIntArray(const std::string& name, int* values, uint32_t count) { m_IntArrayUniforms[name].assign(values, values + count); }
        void AddVec3(const std::string& name, const glm::vec3& value) { m_Vec3Uniforms[name] = value; }
        void AddVec4(const std::string& name, const glm::vec4& value) { m_Vec4Uniforms[name] = value; }
        void AddMat4(const std::string& name, const glm::mat4& value) { m_Mat4Uniforms[name] = value; }

        void AddFlag(MaterialFlag flag) { m_Flags |= (uint32_t)flag; }
        void RemoveFlag(MaterialFlag flag) { m_Flags &= ~(uint32_t)flag; }
        void ToggleFlag(MaterialFlag flag) { m_Flags ^= (uint32_t)flag; }
        bool HasFlag(MaterialFlag flag) const { return m_Flags & (uint32_t)flag; }

        static Ref<Material> Create() { return CreateRef<Material>(); }

    private:
        
        std::unordered_map<std::string, float> m_FloatUniforms;
        std::unordered_map<std::string, int> m_IntUniforms;
        std::unordered_map<std::string, ResourceHandle> m_Textures;
        std::unordered_map<std::string, std::vector<int> > m_IntArrayUniforms;
        std::unordered_map<std::string, glm::vec3> m_Vec3Uniforms;
        std::unordered_map<std::string, glm::vec4> m_Vec4Uniforms;
        std::unordered_map<std::string, glm::mat4> m_Mat4Uniforms;

        uint32_t m_Flags = (uint32_t)MaterialFlag::None;

        static const AssetType GetType() { return AssetType::Material; }
        virtual const AssetType GetAssetType() const override { return AssetType::Material; }
        friend class AssetManager;
    };
}