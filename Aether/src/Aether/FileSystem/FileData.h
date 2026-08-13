#pragma once

namespace Aether {
    struct FileData
    {
        uint8_t* bytes = nullptr;
        size_t size = 0;

        bool IsValid() const { return bytes != nullptr && size > 0; }
        bool Empty() const { return bytes == nullptr || size == 0; }
        
        const uint8_t* Data() const { return bytes; }
        size_t Size() const { return size; }
    };
}