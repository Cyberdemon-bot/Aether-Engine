#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"
#include "Aether/Renderer/Resource.h"
#include "Aether/Renderer/Shader.h"

#include <string>
#include <vector>
#include <string_view>
#include <algorithm>

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

    struct Material : public Asset
    {
        Material() = default;

        void AddImage(std::string_view name, Handle<Asset> handle);
        void AddFloat(std::string_view name, float value);
        void AddInt(std::string_view name, int value);
        void AddIntArray(std::string_view name, int* values, uint32_t count);
        void AddVec3(std::string_view name, const glm::vec3& value);
        void AddVec4(std::string_view name, const glm::vec4& value);
        void AddMat4(std::string_view name, const glm::mat4& value);

        void AddFlag(MaterialFlag flag);
        void RemoveFlag(MaterialFlag flag);
        void ToggleFlag(MaterialFlag flag);
        bool HasFlag(MaterialFlag flag) const;

        std::vector<std::pair<std::string, glm::mat4>> m_Mat4Uniforms;
        std::vector<std::pair<std::string, glm::vec4>> m_Vec4Uniforms;
        std::vector<std::pair<std::string, glm::vec3>> m_Vec3Uniforms;
        std::vector<std::pair<std::string, std::vector<int>>> m_IntArrayUniforms;
        std::vector<std::pair<std::string, Handle<Asset>>> m_Images;
        std::vector<std::pair<std::string, int>> m_IntUniforms;
        std::vector<std::pair<std::string, float>> m_FloatUniforms;

        uint32_t m_Flags = (uint32_t)MaterialFlag::None;
    };

    struct Sheet : public Asset
    {
        Sheet() = default;

        void Resize(uint32_t size)
        {
            BaseHandles.resize(size, Handle<Asset>::MakeInvalid());
            OverrideHandles.resize(size, Handle<Asset>::MakeInvalid());
        }

        void CopyDefaultList(const std::vector<Handle<Asset>>& handleList);
        void CopyOverrideList(const std::vector<Handle<Asset>>& handleList);
        void MoveDefaultList(std::vector<Handle<Asset>>&& handleList);
        void MoveOverrideList(std::vector<Handle<Asset>>&& handleList);

        void Reset();
        void SetOverride(uint32_t index, Handle<Asset> handle);
        void SetDefault(uint32_t index, Handle<Asset> handle);
        void Revert(uint32_t index);
        
        Handle<Asset> GetActiveHandle(uint32_t index) const;
        uint32_t GetSize();

        std::vector<Handle<Asset>> BaseHandles;
        std::vector<Handle<Asset>> OverrideHandles;
    };
}