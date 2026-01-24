#pragma once

#include "aepch.h"
#include "Aether/Core/UUID.h"

namespace Aether {

    struct ShaderProgramSource
    {
        std::string VertexSource;
        std::string FragmentSource;
        std::string GeometrySource;
    };

    class AETHER_API Shader 
    {
    public:
        virtual ~Shader() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void SetInt(const std::string& name, int value) = 0;
        virtual void SetIntArray(const std::string& name,const int* values, uint32_t count) = 0; 
        virtual void SetFloat(const std::string& name, float value) = 0;
        virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
        virtual void SetFloat4(const std::string& name, const glm::vec4& value) = 0;
        virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;

        static Ref<Shader> Create(const std::string& filepath);
    };

    class AETHER_API ShaderLibrary
    {
    public:
        static void Init();
        static void Shutdown();

        static Ref<Shader> Load(const std::string& filepath, UUID id);
        static Ref<Shader> Get(UUID id);
        static bool Exists(UUID id);

    private:
        static std::unordered_map<UUID, Ref<Shader>> s_Shaders;
    };
}