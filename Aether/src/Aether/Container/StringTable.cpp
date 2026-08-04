#include "aepch.h"
#include "Aether/Core/Base.h"
#include "Aether/Container/StringTable.h"

namespace Aether {

    Handle<StringData> StringTable::Commit(std::string_view key, uint64_t hash)
    {
        if (key.size() > std::numeric_limits<uint16_t>::max()) 
        {
            AE_CORE_ERROR("StringTable key {0} is too long - {1} characters", key, key.size());
            return Handle<StringData>::MakeInvalid();
        }
        uint32_t offset = static_cast<uint32_t>(m_Buffer.size());
        uint16_t length = static_cast<uint16_t>(key.size());

        m_Buffer.resize(m_Buffer.size() + sizeof(uint16_t) + key.size());
        std::memcpy(m_Buffer.data() + offset, &length, sizeof(uint16_t));
        std::memcpy(m_Buffer.data() + offset + sizeof(uint16_t), key.data(), key.size());

        Handle<StringData> handle = m_Pool.CreateResource(offset);
        m_Queue.push_back({ handle, hash });
        return handle;
    }

    void StringTable::Init()
    {
        m_Pool.Init();
        m_Map.reserve(32);
        m_Queue.reserve(16);
        m_Buffer.reserve(128);
    }

    void StringTable::Shutdown()
    {
        m_Pool.Shutdown();
        m_Map.clear();
        m_Queue.clear();
        m_Buffer.clear();
    }

    Handle<StringData> StringTable::Get(std::string_view key)
    {
        uint64_t hash = fnv1a_64(key);
        Handle<StringData> handle = Search(key, hash);
        if (handle.IsValid()) return handle;
        return Commit(key, hash);
    }

    Handle<StringData> StringTable::Search(std::string_view key) const
    {
        uint64_t hash = fnv1a_64(key);
        return Search(key, hash);
    }

    Handle<StringData> StringTable::Search(std::string_view key, uint64_t hash) const
    {
        {
            auto it = std::lower_bound(m_Map.begin(), m_Map.end(), hash,
                [](const HashData& entry, uint64_t h) { return entry.hash_code < h; });

            while (it != m_Map.end() && it->hash_code == hash)
            {
                if (GetView(it->handle) == key) return it->handle;
                ++it;
            }
        }

        for (const HashData& entry : m_Queue)
        {
            if (entry.hash_code != hash) continue;
            if (GetView(entry.handle) == key) return entry.handle;
        }

        return Handle<StringData>::MakeInvalid();
    }

    std::string_view StringTable::GetView(Handle<StringData> handle) const
    {
        const uint32_t* offset_ptr = m_Pool.GetResource(handle);
        if (!offset_ptr) return std::string_view{};

        uint32_t offset = *offset_ptr;
        if (offset >= m_Buffer.size()) return std::string_view{};

        const char* ptr = m_Buffer.data() + offset;
        uint16_t length = 0;
        std::memcpy(&length, ptr, sizeof(uint16_t));
        ptr += sizeof(uint16_t);

        return std::string_view(ptr, length);
    }

    std::string StringTable::GetString(Handle<StringData> handle) const
    {
        return std::string(GetView(handle));
    }

    void StringTable::Resolve()
    {
        if (m_Queue.empty()) return;

        m_Map.insert(m_Map.end(), m_Queue.begin(), m_Queue.end());
        m_Queue.clear();

        std::sort(m_Map.begin(), m_Map.end(), [](const HashData& a, const HashData& b)
        {
            return a.hash_code < b.hash_code;
        });
    }
}