#include "Aether/Renderer/Shader.h" 
#include "Platform/OpenGL/OpenGLBase.h"
#include "glm/glm.hpp"

namespace Aether {
    class OpenGLShader : public Shader 
    {
    public:
        OpenGLShader(const std::string& filepath);
        virtual ~OpenGLShader();

        virtual void Bind() const override;
        virtual void Unbind() const override;
        virtual uint32_t GetRendererID() const override { return m_RendererID; }

        virtual void SetInt(const std::string& name, int value) override;
        virtual void SetIntArray(const std::string& name,const int* values, uint32_t count) override; 
        virtual void SetFloat(const std::string& name, float value) override;
        virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
        virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
        virtual void SetMat4(const std::string& name, const glm::mat4& value) override;
        virtual void SetUBOSlot(const std::string& name, int slot) override;

        virtual bool operator==(const Shader& other) const override
		{
			return m_RendererID == other.GetRendererID();
		}
    private:
        unsigned int m_RendererID;  
        std::unordered_map<std::string, int> m_UniformLocationCache;
        std::unordered_map<std::string, int> m_UniformBlockLocationCache;

        int GetUniformLocation(const std::string& name);
        int GetUniformBlockLocation(const std::string& name);
        unsigned int CompileShader(unsigned int type, const std::string& source);
        unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader);
        ShaderProgramSource ParseShader(const std::string& filepath);
    };
}