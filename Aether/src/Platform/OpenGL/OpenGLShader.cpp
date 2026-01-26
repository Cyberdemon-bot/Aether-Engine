#include "Platform/OpenGL/OpenGLShader.h"
#include <glm/gtc/type_ptr.hpp>

namespace Aether {

    OpenGLShader::OpenGLShader(const std::string& filepath)
        : m_FilePath(filepath), m_RendererID(0)
    {
        ShaderProgramSource source = ParseShader(filepath);
        m_RendererID = CreateShader(source.VertexSource, source.FragmentSource, source.GeometrySource);
    }

    OpenGLShader::~OpenGLShader()
    {
        GLCall(glDeleteProgram(m_RendererID));
    }


    void OpenGLShader::Bind() const
    {
        GLCall(glUseProgram(m_RendererID));
    }

    void OpenGLShader::Unbind() const
    {
        GLCall(glUseProgram(0));
    }


    void OpenGLShader::SetInt(const std::string& name, int value)
    {
        GLCall(glUniform1i(GetUniformLocation(name), value));
    }

    void OpenGLShader::SetIntArray(const std::string& name,const int* values, uint32_t count)
    {
        GLCall(glUniform1iv(GetUniformLocation(name), count, values));
    }

    void OpenGLShader::SetFloat(const std::string& name, float value)
    {
        GLCall(glUniform1f(GetUniformLocation(name), value));
    }

    void OpenGLShader::SetFloat3(const std::string& name, const glm::vec3& value)
    {
        GLCall(glUniform3f(GetUniformLocation(name), value.x, value.y, value.z));
    }

    void OpenGLShader::SetFloat4(const std::string& name, const glm::vec4& value)
    {
        GLCall(glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w));
    }

    void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value)
    {
        GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value)));
    }


    int OpenGLShader::GetUniformLocation(const std::string& name)
    {
        if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
            return m_UniformLocationCache[name];

        GLCall(int location = glGetUniformLocation(m_RendererID, name.c_str()));
        if (location == -1) AE_CORE_TRACE("Warning: uniform '{0}' doesn't exist!", name, location); 

        m_UniformLocationCache[name] = location;
        return location;
    }

    ShaderProgramSource OpenGLShader::ParseShader(const std::string& filepath)
    {
        std::ifstream stream(filepath);

        enum class ShaderType
        {
            NONE = -1, VERTEX = 0, FRAGMENT = 1, GEOMETRY = 2
        };

        std::string line;
        std::stringstream ss[3]; 
        ShaderType type = ShaderType::NONE;

        while (getline(stream, line))
        {
            if (line.find("#shader") != std::string::npos)
            {
                if (line.find("vertex") != std::string::npos)
                    type = ShaderType::VERTEX;
                else if (line.find("fragment") != std::string::npos)
                    type = ShaderType::FRAGMENT;
                else if (line.find("geometry") != std::string::npos)
                    type = ShaderType::GEOMETRY;
            }
            else
            {
                if (type != ShaderType::NONE)
                    ss[(int)type] << line << '\n';
            }
        }

        return { ss[0].str(), ss[1].str(), ss[2].str() };
    }

    unsigned int OpenGLShader::CompileShader(unsigned int type, const std::string& source)
    {
        unsigned int id;
        GLCall(id = glCreateShader(type));
        const char* src = source.c_str();
        GLCall(glShaderSource(id, 1, &src, nullptr));
        GLCall(glCompileShader(id));

        int result;
        GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
        if (result == GL_FALSE)
        {
            int length;
            GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
            
            std::vector<char> message(length);
            GLCall(glGetShaderInfoLog(id, length, &length, message.data()));
            
            std::string shaderType;
            if (type == GL_VERTEX_SHADER) shaderType = "Vertex";
            else if (type == GL_FRAGMENT_SHADER) shaderType = "Fragment";
            else if (type == GL_GEOMETRY_SHADER) shaderType = "Geometry";

            AE_CORE_ERROR("Failed to compile {0} shader!", shaderType);
            AE_CORE_ERROR("{0}", message.data());
            
            GLCall(glDeleteShader(id));
            return 0;
        }

        return id;
    }

    unsigned int OpenGLShader::CreateShader(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader)
    {
        unsigned int program;
        GLCall(program = glCreateProgram());
        
        unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
        unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);
        
        unsigned int gs = 0;
        bool hasGeometry = !geometryShader.empty();
        if (hasGeometry) 
            gs = CompileShader(GL_GEOMETRY_SHADER, geometryShader);

        GLCall(glAttachShader(program, vs));
        GLCall(glAttachShader(program, fs));
        if (hasGeometry && gs != 0) {GLCall(glAttachShader(program, gs));}

        GLCall(glLinkProgram(program));
        GLCall(glValidateProgram(program));

        GLCall(glDeleteShader(vs));
        GLCall(glDeleteShader(fs));
        if (hasGeometry && gs != 0) 
            GLCall(glDeleteShader(gs));

        return program;
    }

}