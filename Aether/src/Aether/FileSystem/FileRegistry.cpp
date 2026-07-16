#include "aepch.h"
#include "Aether/Core/Base.h"
#include "Aether/FileSystem/FileRegistry.h"

namespace Aether {

    void FileRegistry::Init()
    {
        m_Map.reserve(16);
        m_Pool.reserve(128);
    }

    void FileRegistry::Shutdown()
    {
        m_Map.clear();
        m_Pool.clear();
    }

    void FileRegistry::RegisterPath(std::string_view virtual_path)
    {
        uint64_t hash_code = fnv1a_64(virtual_path);
        uint32_t offset = static_cast<uint32_t>(m_Pool.size());
        uint16_t psize = static_cast<uint16_t>(virtual_path.size());
        const char* bsize = reinterpret_cast<const char*>(&psize);
        m_Pool.push_back(bsize[0]);
        m_Pool.push_back(bsize[1]);
        m_Pool.insert(m_Pool.end(), virtual_path.begin(), virtual_path.end());
        m_Pool.push_back('\0');
        m_Map.push_back({hash_code, offset, Handle<FileData>::MakeInvalid()});
    }

    void FileRegistry::CommitTable()
    {
        std::sort(m_Map.begin(), m_Map.end(), [](const PathEntry& a, const PathEntry& b)
        {
            return a.hash_code < b.hash_code;
        });

        auto last = std::unique(m_Map.begin(), m_Map.end(), [](const PathEntry& a, const PathEntry& b)
        {
            return a.hash_code == b.hash_code;
        });

        m_Map.erase(last, m_Map.end());
        m_Map.shrink_to_fit();
        m_Pool.shrink_to_fit();
    }

    PathEntry* FileRegistry::Query(uint64_t hash_code)
    {   
        auto it = std::lower_bound(m_Map.begin(), m_Map.end(), hash_code, [](const PathEntry& entry, uint64_t hash)
        {
            return entry.hash_code < hash;
        });

        if (it != m_Map.end() && it->hash_code == hash_code) return &(*it);
        AE_CORE_ERROR("Path {0} is not found!", hash_code);
        return nullptr;
    }

    std::string_view FileRegistry::GetView(uint32_t offset) const
    {
        if (offset >= m_Pool.size()) return std::string_view{};
        const char* ptr = m_Pool.data() + offset;
        uint16_t psize = 0;
        std::memcpy(&psize, ptr, sizeof(uint16_t));
        ptr = ptr + sizeof(uint16_t);
        return std::string_view(ptr, psize);
    }
}