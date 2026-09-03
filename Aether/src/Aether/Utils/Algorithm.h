#pragma once

#include <span>
#include <cstdint>
#include <utility>

namespace Aether {
    template <typename T, typename Func>
    bool RadixSort64(std::span<T> data, std::span<T> temp, Func&& keyExtractor)
    {
        if (temp.size() < data.size()) return false;
        if (data.size() <= 1) return true;

        const uint32_t n = static_cast<uint32_t>(data.size());

        uint32_t hist[8][256] = {};
        for (uint32_t i = 0; i < n; i++)
        {
            uint64_t k = keyExtractor(data[i]);
            for (int p = 0; p < 8; p++)
                hist[p][(k >> (p * 8)) & 0xFF]++;
        }

        std::span<T> src = data;
        std::span<T> dst = temp;
        bool is_in_temp = false; 

        for (int p = 0; p < 8; p++)
        {
            uint32_t* h = hist[p];
            bool skip = false;
            for (int b = 0; b < 256; b++)
                if (h[b] == n) { skip = true; break; }
            if (skip) continue;

            uint32_t offset[256];
            offset[0] = 0;
            for (int b = 1; b < 256; b++)
                offset[b] = offset[b - 1] + h[b - 1];
            for (uint32_t i = 0; i < n; i++)
            {
                uint32_t bucket = (keyExtractor(src[i]) >> (p * 8)) & 0xFF;
                dst[offset[bucket]++] = std::move(src[i]);
            }

            std::swap(src, dst);
            is_in_temp = !is_in_temp;
        }

        if (is_in_temp) std::move(temp.begin(), temp.begin() + n, data.begin());

        return true;
    }
}