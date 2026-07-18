#include "aepch.h"
#include "Aether/Core/Base.h"
#include "Aether/Core/Assert.h"
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
        if (virtual_path.size() > 0xFFFF) 
        {
            AE_CORE_ERROR("Path length ({0}) exceeds maximum allowed 65535 bytes!", virtual_path.size());
            return; 
        }

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

    bool FileRegistry::CompareString(uint32_t offset_a, uint32_t offset_b)
    {
        std::string_view view_a = GetView(offset_a); 
        std::string_view view_b = GetView(offset_b); 
        if (view_a.size() != view_b.size()) return false;
        
        for (size_t i = 0; i < view_a.size(); ++i) 
            if (view_a[i] != view_b[i]) return false;
        return true;
    }

    void FileRegistry::CommitTable()
    {
        std::sort(m_Map.begin(), m_Map.end(), [](const PathEntry& a, const PathEntry& b)
        {
            return a.hash_code < b.hash_code;
        });

        for (size_t i = 1; i < m_Map.size(); i++)
            if (m_Map[i].hash_code == m_Map[i - 1].hash_code && 
                !CompareString(m_Map[i].byte_offset, m_Map[i - 1].byte_offset))
            {
                std::string_view pathA = GetView(m_Map[i - 1].byte_offset);
                std::string_view pathB = GetView(m_Map[i].byte_offset); 
                AE_CORE_ERROR("COLLISION: '{0}' and '{1}' have the same hash {2}!", pathA, pathB, m_Map[i].hash_code);
                AE_CORE_ASSERT(false, "Hash code collision detected!");
            }

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