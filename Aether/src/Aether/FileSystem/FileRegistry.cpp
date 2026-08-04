#include "aepch.h"
#include "Aether/Core/Base.h"
#include "Aether/Core/Assert.h"
#include "Aether/FileSystem/FileRegistry.h"

namespace Aether {

    void FileRegistry::Init()
    {
        m_Strings.Init();
        m_Entries.reserve(16);
    }

    void FileRegistry::Shutdown()
    {
        m_Strings.Shutdown();
        m_Entries.clear();
    }

    void FileRegistry::RegisterPath(std::string_view virtual_path)
    {
        Handle<StringData> string_handle = m_Strings.Get(virtual_path);
        if (!string_handle.IsValid()) return; 

        uint32_t idx = string_handle.index;
        if (idx >= m_Entries.size())
            m_Entries.resize(idx + 1);

        if (m_Entries[idx].string_handle.Blend() == string_handle.Blend()) return;
        m_Entries[idx] = { string_handle, Handle<FileData>::MakeInvalid() };
    }

    void FileRegistry::CommitTable()
    {
        m_Strings.Resolve();
    }

    PathEntry* FileRegistry::Query(std::string_view virtual_path)
    {
        Handle<StringData> string_handle = m_Strings.Search(virtual_path);
        if (!string_handle.IsValid())
        {
            AE_CORE_ERROR("FileRegistry::Query: '{}' is not registered!", virtual_path);
            return nullptr;
        }

        uint32_t idx = string_handle.index;
        if (idx >= m_Entries.size())
        {
            AE_CORE_ERROR("FileRegistry::Query: handle index {} out of range!", idx);
            return nullptr;
        }

        if (m_Entries[idx].string_handle.Blend() != string_handle.Blend())
        {
            AE_CORE_ERROR("FileRegistry::Query: stale handle for '{}'!", virtual_path);
            return nullptr;
        }

        return &m_Entries[idx];
    }

    std::string_view FileRegistry::GetView(const PathEntry& entry) const
    {
        return m_Strings.GetView(entry.string_handle);
    }
}