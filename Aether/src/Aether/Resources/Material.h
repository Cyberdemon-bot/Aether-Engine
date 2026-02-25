#pragma once
#include "aepch.h"
#include "Aether/Renderer/Texture.h"
#include "Aether/Renderer/Shader.h"
#include "Aether/Core/UUID.h"

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
        Material() {};
        static Ref<Material> Create() { return CreateRef<Material>(); }

        void UploadMaterial(Ref<Shader> shader, uint32_t startSlot = 0);

        void AddTexture(const std::string& name, Ref<Texture2D> texture) { m_Textures[name] = texture; }
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
    private:
        
        std::unordered_map<std::string, float> m_FloatUniforms;
        std::unordered_map<std::string, int> m_IntUniforms;
        std::unordered_map<std::string, Ref<Texture2D>> m_Textures;
        std::unordered_map<std::string, std::vector<int> > m_IntArrayUniforms;
        std::unordered_map<std::string, glm::vec3> m_Vec3Uniforms;
        std::unordered_map<std::string, glm::vec4> m_Vec4Uniforms;
        std::unordered_map<std::string, glm::mat4> m_Mat4Uniforms;

        uint32_t m_Flags = (uint32_t)MaterialFlag::None;
    };

    class AETHER_API MaterialLibrary
    {
    public:
        static void Init();
        static void Shutdown();

        static void Add(Ref<Material> obj, UUID id);
        static Ref<Material> Get(UUID id);

        static bool Exists(UUID id);
    private:
        static std::unordered_map<UUID, Ref<Material>>& GetMaterials();
    };
}