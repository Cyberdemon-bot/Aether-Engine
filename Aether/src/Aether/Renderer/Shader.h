#pragma once

#include "aepch.h"

namespace Aether {

    struct ShaderProgramSource
    {
        std::string VertexSource;
        std::string FragmentSource;
        std::string GeometrySource;
    };

    class Shader 
    {
    public:
        virtual ~Shader() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void SetInt(const std::string& name, int value) = 0;
        virtual void SetIntArray(const std::string& name, int* values, uint32_t count) = 0; 
        virtual void SetFloat(const std::string& name, float value) = 0;
        virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
        virtual void SetFloat4(const std::string& name, const glm::vec4& value) = 0;
        virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;

        static Ref<Shader> Create(const std::string& filepath);
    };

    class ShaderLibrary
    {
    public:
        Ref<Shader> Load(const std::string& name, const std::string& filepath);
        Ref<Shader> Get(const std::string& name);
        
        bool Exists(const std::string& name) const;
    private:
        std::unordered_map<std::string, Ref<Shader>> m_Shaders;
        Ref<Shader> m_ErrorShader = Shader::Create("assets/shaders/Basic.shader");
    };
}