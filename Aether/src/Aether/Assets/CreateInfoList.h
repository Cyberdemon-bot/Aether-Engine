#pragma once

#include <span>
#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include "Aether/Core/Assert.h"
#include "Aether/Assets/Asset.h"
#include "Aether/Assets/AssetCreateInfo.h"

namespace Aether {
 
    struct AssetRange
    {
        AssetType Kind;
        uint32_t Offset; 
        uint32_t Count;
    };
 
    using AssetVariant = std::variant<
        AImageCreateInfo,
        AMaterialCreateInfo,
        ASheetCreateInfo,
        ASkeletonCreateInfo,
        AClipCreateInfo,
        AMeshCreateInfo
    >;
 
    class CreateInfoList
    {
    public:
        virtual ~CreateInfoList() = default;
        size_t GetAssetCount() const { return m_Assets.size(); }
 
        template<typename T, typename Fn>
        void ForEach(AssetType kind, Fn&& fn) const
        {
            for (const AssetRange& r : m_Ranges)
            {
                if (r.Kind != kind)
                    continue;
 
                for (uint32_t i = r.Offset; i < r.Offset + r.Count; ++i)
                {
                    const T* item = std::get_if<T>(&m_Assets[i]);
                    AE_CORE_ASSERT(item != nullptr, "CreateInfoList: range/variant kind mismatch");
                    fn(*item);
                }
                return; 
            }
        }
 
        const std::string& GetFilePath() const { return m_FilePath; }
 
    protected:
        std::string m_FilePath;
 
        std::vector<AssetVariant> m_Assets;   
        std::vector<AssetRange> m_Ranges;  
 
        void ReserveAssets(size_t total) { m_Assets.reserve(total); }

        template<typename T, typename MakeFn>
        void AppendKind(AssetType kind, size_t count, MakeFn&& make)
        {
            uint32_t offset = static_cast<uint32_t>(m_Assets.size());
            for (size_t i = 0; i < count; ++i)
                m_Assets.emplace_back(std::in_place_type<T>, make(i));
 
            if (count > 0)
                m_Ranges.push_back({ kind, offset, static_cast<uint32_t>(count) });
        }
    };
}