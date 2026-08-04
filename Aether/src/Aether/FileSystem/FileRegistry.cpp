#include "aepch.h"
#include "Aether/Core/Base.h"
#include "Aether/Core/Assert.h"
#include "Aether/FileSystem/FileRegistry.h"

namespace Aether {

    void FileRegistry::Init()
    {
        m_Table.Init();
    }

    void FileRegistry::Shutdown()
    {
        m_Table.Shutdown();
    }

    void FileRegistry::RegisterPath(std::string_view virtual_path)
    {
        m_Table.GetOrCreate(virtual_path);
    }

    void FileRegistry::CommitTable()
    {
        m_Table.Resolve();
    }

    PathEntry* FileRegistry::Query(std::string_view virtual_path)
    {
        Handle<PathEntry> handle = m_Table.Search(virtual_path);
        if (!handle.IsValid())
        {
            AE_CORE_ERROR("FileRegistry::Query: '{}' is not registered!", virtual_path);
            return nullptr;
        }

        return m_Table.GetData(handle);
    }

    std::string_view FileRegistry::GetView(const Handle<PathEntry>& handle) const
    {
        return m_Table.GetView(handle);
    }
}