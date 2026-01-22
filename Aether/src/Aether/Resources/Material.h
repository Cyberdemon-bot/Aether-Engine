#pragma once
#include "aepch.h"
#include "Aether/Resources/Texture.h"
#include "Aether/Resources/Shader.h"

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

    class AETHER_API Material 
    {
    public:
        Material(const Ref<Shader>& shader);

        void Bind(uint32_t startSlot = 0);
        void Unbind();
        void UploadMaterial();

        Ref<Shader> GetShader() const { return m_Shader; }
        Ref<Texture2D> GetTexture(const std::string& name) const;

        void SetTexture(const std::string& name, const Ref<Texture2D>& texture);

        void SetFloat(const std::string& name, float value);
        void SetInt(const std::string& name, int value);
        void SetIntArray(const std::string& name, int* values, uint32_t count);
        
        void SetFloat3(const std::string& name, const glm::vec3& value);
        void SetFloat4(const std::string& name, const glm::vec4& value);
        void SetMat4(const std::string& name, const glm::mat4& value);

        void SetFlags(uint32_t flags) { m_Flags = flags; }
        uint32_t GetFlags() const { return m_Flags; }
    private:
        Ref<Shader> m_Shader;

        std::unordered_map<std::string, Ref<Texture2D>> m_Textures;
        std::unordered_map<std::string, float> m_FloatUniforms;
        std::unordered_map<std::string, int> m_IntUniforms;
        
        std::unordered_map<std::string, std::vector<int> > m_IntArrayUniforms;
        std::unordered_map<std::string, glm::vec3> m_Vec3Uniforms;
        std::unordered_map<std::string, glm::vec4> m_Vec4Uniforms;

        std::unordered_map<std::string, glm::mat4> m_Mat4Uniforms;

        uint32_t m_Flags = 0;
    };

    class MateriaLibrary
    {

    };
}