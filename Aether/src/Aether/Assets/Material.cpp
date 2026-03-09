#include "aepch.h"
#include "Aether/Assets/Material.h"

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
}