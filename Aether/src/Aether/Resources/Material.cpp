#include "Aether/Resources/Material.h"

namespace Aether {
    Material::Material(const Ref<Shader>& shader)
        : m_Shader(shader)
    {AE_CORE_ASSERT(shader, "Shader cannot be null!");}


    void Material::Bind(uint32_t startSlot)
    {
        m_Shader->Bind();

        for (const auto& [name, texture] : m_Textures)
        {
            texture->Bind(startSlot);
            m_Shader->SetInt(name, startSlot);
            startSlot++;
        }
    }

    void Material::UploadMaterial()
    {
        m_Shader->Bind();

        for (const auto& [name, value] : m_FloatUniforms) m_Shader->SetFloat(name, value);

        for (const auto& [name, value] : m_IntUniforms) m_Shader->SetInt(name, value);

        for (const auto& [name, intVector] : m_IntArrayUniforms) m_Shader->SetIntArray(name, intVector.data(), (uint32_t)intVector.size());

        for (const auto& [name, value] : m_Vec3Uniforms) m_Shader->SetFloat3(name, value);

        for (const auto& [name, value] : m_Vec4Uniforms) m_Shader->SetFloat4(name, value);

        for (const auto& [name, value] : m_Mat4Uniforms) m_Shader->SetMat4(name, value);
    }

    void Material::Unbind()
    {
        m_Shader->Unbind();
    }

    void Material::SetTexture(const std::string& name, const Ref<Texture2D>&  texture)
    {
        m_Textures[name] = texture;
    }

    void Material::SetFloat(const std::string& name, float value)
    {
        m_FloatUniforms[name] = value;
    }

    void Material::SetInt(const std::string& name, int value)
    {
        m_IntUniforms[name] = value;
    }

    void Material::SetIntArray(const std::string& name, int* values, uint32_t count)
    {
        m_IntArrayUniforms[name].assign(values, values + count);
    }

    void Material::SetFloat3(const std::string& name, const glm::vec3& value)
    {
        m_Vec3Uniforms[name] = value;
    }

    void Material::SetFloat4(const std::string& name, const glm::vec4& value)
    {
        m_Vec4Uniforms[name] = value;
    }

    void Material::SetMat4(const std::string& name, const glm::mat4& value)
    {
        m_Mat4Uniforms[name] = value;
    }
}