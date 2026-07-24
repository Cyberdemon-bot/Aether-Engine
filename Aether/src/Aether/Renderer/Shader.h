#pragma once

#include "glm/glm.hpp"
#include "Aether/Renderer/Resource.h"

namespace Aether {

    struct ShaderProgramSource
    {
        std::string VertexSource;
        std::string FragmentSource;
        std::string GeometrySource;
    };

    class AETHER_API Shader : public Resource
    {
    public:
        virtual ~Shader() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;
        virtual uint32_t GetRendererID() const = 0;

        virtual void SetInt(const std::string& name, int value) = 0;
        virtual void SetIntArray(const std::string& name,const int* values, uint32_t count) = 0; 
        virtual void SetFloat(const std::string& name, float value) = 0;
        virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
        virtual void SetFloat4(const std::string& name, const glm::vec4& value) = 0;
        virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;
        virtual void SetUBOSlot(const std::string& name, int slot) = 0;

        template<typename... Args>
        static Ref<Shader> Create(Args&&... args)
        {
            Scope<Shader> scope = CreateImpl(std::forward<Args>(args)...);
            return Ref<Shader>(std::move(scope));
        }

        virtual bool operator==(const Shader& other) const = 0;

    private:
		static Scope<Shader> CreateImpl(const std::string& filepath);
        static Scope<Shader> CreateImpl(const ShaderProgramSource& source);

		friend class ResourceManager;
    };
}