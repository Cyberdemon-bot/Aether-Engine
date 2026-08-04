#pragma once

#include <string_view>
#include <vector>
#include "Aether/Container/Handle.h"
#include "Aether/Container/StringTable.h"
#include "Aether/FileSystem/FileData.h"

namespace Aether {

    class FileSystem;

    struct PathEntry
    {
        Handle<StringData> string_handle;  
        Handle<FileData> file_handle;   
    };

    class FileRegistry
    {
    public:
        void Init();
        void Shutdown();

        void RegisterPath(std::string_view virtual_path);
        void CommitTable();
    private:
        PathEntry* Query(std::string_view virtual_path);
        std::string_view GetView(const PathEntry& entry) const;

        StringTable m_Strings;
        std::vector<PathEntry> m_Entries;

        friend class FileSystem;
    };
}