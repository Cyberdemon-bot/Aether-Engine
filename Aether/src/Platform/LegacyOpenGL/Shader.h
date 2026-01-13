#pragma once

#include <string>
#include <unordered_map>

#include "glm/glm.hpp"

namespace Aether::Legacy {
    struct ShaderProgramSource
    {
        std::string VertexSource;
        std::string FragmentSource;
    };

    class AETHER_API Shader
    {
    private:
        std::string m_FilePath;
        unsigned int m_RendererID;  
        std::unordered_map<std::string, int> m_UniformLocationCache;

        int GetUniformLocation(const std::string& name);
        unsigned int CompileShader(unsigned int type, const std::string& source);
        unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader);
        ShaderProgramSource ParseShader(const std::string& filepath);

    public:
        Shader(const std::string& filepath);
        ~Shader();

        void Bind() const;
        void Unbind() const;

        void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
        void SetUniform3f(const std::string& name, const glm::vec3& value);
        void SetUniform1f(const std::string& name, float value);
        void SetUniform1i(const std::string& name, int value);
        void SetUniformMat4f(const std::string& name, const glm::mat4& matrix);
        void SetUniformMat4fv(const std::string& name, unsigned int count, const float* value);
    };
}