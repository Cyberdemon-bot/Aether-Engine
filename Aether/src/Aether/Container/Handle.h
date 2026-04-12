#pragma once 

namespace Aether {
    template<typename Tag>
    struct Handle
    {
        uint32_t index = UINT32_MAX;
        uint32_t generation = 0;
        bool IsValid() const { return index != UINT32_MAX; }
        static Handle MakeInvalid() { return {UINT32_MAX, 0}; }
        uint64_t Blend() const { return ((uint64_t)index << 32) | generation; }
    };
}