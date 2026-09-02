#pragma once

#include <vector>
#include <array>
#include <span>
#include <cstdint>
#include "Aether/Assets/Asset.h"

namespace Aether {
    struct BatchRegisterResult
    {
    public:
        BatchRegisterResult() = default;

        std::span<const UUID> Get(AssetType type) const
        {
            const auto& slice = m_Slices[static_cast<size_t>(type)];
            if (slice.count == 0)
                return {};

            return { m_AllIDs.data() + slice.offset, slice.count };
        }

        bool Empty() const { return m_AllIDs.empty(); }
        size_t TotalCount() const { return m_AllIDs.size(); }

    private:
        struct Slice
        {
            uint32_t offset = 0;
            uint32_t count  = 0;
        };

        std::vector<UUID> m_AllIDs;
        std::array<Slice, static_cast<size_t>(AssetType::Count)> m_Slices{};

        friend class AssetRegister;
    };
}