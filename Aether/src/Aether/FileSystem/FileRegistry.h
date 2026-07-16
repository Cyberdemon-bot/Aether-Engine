#pragma once

#include <string_view>
#include <vector>
#include "Aether/Container/Handle.h"
#include "Aether/FileSystem/FileData.h"

namespace Aether {

    class FileSystem;
    struct PathEntry
    {
        uint64_t hash_code;
        uint32_t byte_offset;
        Handle<FileData> handle;
    };
    
    class FileRegistry
    {
    public:
        void Init();
        void Shutdown();
        void RegisterPath(std::string_view virtual_path);
        void CommitTable();
    private:
        PathEntry* Query(uint64_t hash_code);
        bool CompareString(uint32_t offset_a, uint32_t offset_b);
        std::string_view GetView(uint32_t offset) const;
        std::vector<PathEntry> m_Map;
        std::vector<char> m_Pool;
        friend class FileSystem;
    };
}