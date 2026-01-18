// ============================================================================
// Aether/src/Aether/Renderer/Material.h
// ============================================================================
#pragma once
#include "aepch.h"
#include "Shader.h"
#include "Texture.h"

namespace Aether {

    // Material property types that can be stored
    enum class MaterialPropertyType
    {
        None = 0,
        Float,
        Float2,
        Float3,
        Float4,
        Int,
        Bool,
        Texture2D,
        TextureCube
    };

    // Stores a single material property (uniform value or texture)
    struct MaterialProperty
    {
        std::string Name;
        MaterialPropertyType Type;
        
        // Union to store different value types
        union
        {
            float FloatValue;
            glm::vec2 Float2Value;
            glm::vec3 Float3Value;
            glm::vec4 Float4Value;
            int IntValue;
            bool BoolValue;
        };
        
        // Textures stored separately (can't go in union)
        Ref<Texture2D> Texture2DValue;
        Ref<TextureCube> TextureCubeValue;

        MaterialProperty() : Type(MaterialPropertyType::None), FloatValue(0.0f) {}
    };

    // Material flags for render states
    enum class MaterialFlag : uint32_t
    {
        None = 0,
        DepthTest = BIT(0),
        DepthWrite = BIT(1),
        Blend = BIT(2),
        TwoSided = BIT(3),
        Wireframe = BIT(4)
    };

    // Main Material class
    class Material
    {
    public:
        Material(Ref<Shader> shader, const std::string& name = "Material");
        ~Material() = default;

        // Bind material (sets all uniforms and textures)
        void Bind();
        void Unbind();

        // === Property Setters (Generic) ===
        void Set(const std::string& name, float value);
        void Set(const std::string& name, const glm::vec2& value);
        void Set(const std::string& name, const glm::vec3& value);
        void Set(const std::string& name, const glm::vec4& value);
        void Set(const std::string& name, int value);
        void Set(const std::string& name, bool value);
        void Set(const std::string& name, Ref<Texture2D> texture);
        void Set(const std::string& name, Ref<TextureCube> texture);

        // === Property Getters ===
        template<typename T>
        T Get(const std::string& name) const;

        float GetFloat(const std::string& name) const;
        glm::vec3 GetFloat3(const std::string& name) const;
        Ref<Texture2D> GetTexture2D(const std::string& name) const;

        // === Convenience Setters for Common PBR Properties ===
        void SetAlbedo(const glm::vec3& color) { Set("u_AlbedoColor", color); }
        void SetMetallic(float value) { Set("u_Metallic", value); }
        void SetRoughness(float value) { Set("u_Roughness", value); }
        void SetEmission(const glm::vec3& color) { Set("u_Emission", color); }
        
        void SetAlbedoMap(Ref<Texture2D> texture) { Set("u_AlbedoMap", texture); }
        void SetNormalMap(Ref<Texture2D> texture) { Set("u_NormalMap", texture); }
        void SetMetallicMap(Ref<Texture2D> texture) { Set("u_MetallicMap", texture); }
        void SetRoughnessMap(Ref<Texture2D> texture) { Set("u_RoughnessMap", texture); }

        // === Flags ===
        void SetFlag(MaterialFlag flag, bool value = true);
        bool GetFlag(MaterialFlag flag) const;
        void SetFlags(uint32_t flags) { m_Flags = flags; }
        uint32_t GetFlags() const { return m_Flags; }

        // === Shader Access ===
        Ref<Shader> GetShader() const { return m_Shader; }
        void SetShader(Ref<Shader> shader) { m_Shader = shader; }

        // === Metadata ===
        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        // === Utilities ===
        bool HasProperty(const std::string& name) const;
        const std::unordered_map<std::string, MaterialProperty>& GetProperties() const { return m_Properties; }

        // Create a copy of this material
        Ref<Material> Clone() const;

    private:
        void ApplyRenderStates();
        void BindTextures();
        void SetUniforms();

    private:
        std::string m_Name;
        Ref<Shader> m_Shader;
        
        // All material properties stored here
        std::unordered_map<std::string, MaterialProperty> m_Properties;
        
        // Render state flags
        uint32_t m_Flags = (uint32_t)MaterialFlag::DepthTest | (uint32_t)MaterialFlag::DepthWrite;
        
        // Texture slot counter (reset each frame)
        mutable uint32_t m_CurrentTextureSlot = 0;
    };

    // ========================================================================
    // Material Instance - Lightweight material that shares a shader
    // ========================================================================
    class MaterialInstance
    {
    public:
        MaterialInstance(Ref<Material> baseMaterial);
        
        void Bind();
        
        // Override base material properties
        template<typename T>
        void Set(const std::string& name, const T& value);
        
        Ref<Material> GetBaseMaterial() const { return m_BaseMaterial; }
        
    private:
        Ref<Material> m_BaseMaterial;
        std::unordered_map<std::string, MaterialProperty> m_Overrides;
    };

    // ========================================================================
    // Material Library - Manages all materials in the project
    // ========================================================================
    class MaterialLibrary
    {
    public:
        MaterialLibrary() = default;
        ~MaterialLibrary() = default;

        // Add material to library
        void Add(const std::string& name, Ref<Material> material);
        
        // Create material from shader
        Ref<Material> Create(const std::string& name, Ref<Shader> shader);
        
        // Load material from file (JSON/YAML)
        Ref<Material> Load(const std::string& filepath);
        
        // Get material by name
        Ref<Material> Get(const std::string& name);
        
        // Check if material exists
        bool Exists(const std::string& name) const;
        
        // Remove material
        void Remove(const std::string& name);
        
        // Get all materials
        const std::unordered_map<std::string, Ref<Material>>& GetAll() const { return m_Materials; }
        
        // Create default materials
        void CreateDefaults();

    private:
        std::unordered_map<std::string, Ref<Material>> m_Materials;
    };

    // ========================================================================
    // Material Presets - Factory methods for common materials
    // ========================================================================
    class MaterialPresets
    {
    public:
        // Standard PBR material
        static Ref<Material> CreatePBR(const std::string& name = "PBR Material");
        
        // Unlit/flat color material
        static Ref<Material> CreateUnlit(const std::string& name = "Unlit Material");
        
        // Standard material with default textures
        static Ref<Material> CreateStandard(
            const glm::vec3& albedo = glm::vec3(0.8f),
            float metallic = 0.0f,
            float roughness = 0.5f
        );
        
        // Create from texture set
        static Ref<Material> CreateFromTextures(
            Ref<Texture2D> albedo,
            Ref<Texture2D> normal = nullptr,
            Ref<Texture2D> metallic = nullptr,
            Ref<Texture2D> roughness = nullptr
        );
    };
}