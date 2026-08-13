#pragma once

#include <string_view>
#include "Aether/Container/Handle.h"
#include "Aether/Container/StringTable.h"
#include "Aether/FileSystem/FileData.h"

namespace Aether {

    class FileSystem;

    struct PathEntry
    {
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
        StringTable<Handle<PathEntry>, PathEntry> m_Table;

        friend class FileSystem;
    };
}