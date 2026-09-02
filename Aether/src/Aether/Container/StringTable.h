#pragma once

#include <span>
#include <vector>
#include <string_view>
#include "Aether/Core/Base.h"
#include "Aether/Core/Log.h"
#include "Aether/Utils/Algorithm.h"
#include "Aether/Container/Handle.h"
#include "Aether/Container/ResourcePool.h"

namespace Aether {

    template<typename HandleType, typename DataType>
    class StringTable
    {
    public:
        struct TableElement
        {
            uint32_t byte_offset;
            DataType data;

            template<typename... Args>
            TableElement(uint32_t offset, Args&&... args)
                : byte_offset(offset), data(std::forward<Args>(args)...) {}
        };

        struct HashData
        {
            uint32_t byte_offset;
            uint64_t hash_code;
            HandleType handle;
        };

        void Init()
        {
            m_Pool.Init();
            m_Map.reserve(32);
            m_Temp.reserve(16);
            m_SortedSize = 0;
            m_Buffer.reserve(128);
        }

        void Shutdown()
        {
            m_Pool.Shutdown();
            m_Map.clear(); m_Map.shrink_to_fit();
            m_Temp.clear(); m_Temp.shrink_to_fit();
            m_SortedSize = 0;
            m_Buffer.clear(); m_Buffer.shrink_to_fit();
        }

        HandleType Search(std::string_view key) const
        {
            uint64_t hash = fnv1a_64(key);
            const HashData* it = Search(key, hash);
            if (!it) return HandleType::Null();
            return it->handle;
        }

        template<typename... Args>
        HandleType SafeCreate(std::string_view key, Args&&... args)
        {
            uint64_t hash = fnv1a_64(key);
            if (Search(key, hash) != nullptr) return HandleType::Null();
            return Commit(key, hash, std::forward<Args>(args)...);
        }

        template<typename... Args>
        HandleType GetOrCreate(std::string_view key, Args&&... args)
        {
            uint64_t hash = fnv1a_64(key);
            HashData* entry = Search(key, hash);

            if (entry)
            {
                if (entry->handle.IsValid() && m_Pool.GetResource(entry->handle))
                    return entry->handle;

                HandleType handle = m_Pool.CreateResource(entry->byte_offset, std::forward<Args>(args)...);
                entry->handle = handle;
                return handle;
            }

            return Commit(key, hash, std::forward<Args>(args)...);
        }

        void Destroy(HandleType handle)
        {
            if (!m_Pool.GetResource(handle)) return;
            m_Pool.DestroyResource(handle);
        }

        void Destroy(std::string_view key)
        {
            uint64_t hash = fnv1a_64(key);
            HashData* entry = Search(key, hash);
            if (!entry) return;
            Destroy(entry->handle);
            entry->handle = HandleType::Null();
        }

        std::string_view GetView(HandleType handle) const
        {
            const TableElement* it = m_Pool.GetResource(handle);
            if (!it) return std::string_view{};
            return CalcView(it->byte_offset);
        }

        std::string GetString(HandleType handle) const
        {
            const TableElement* it = m_Pool.GetResource(handle);
            if (!it) return std::string{};
            return std::string(CalcView(it->byte_offset));
        }

        DataType* GetData(HandleType handle)
        {
            TableElement* it = m_Pool.GetResource(handle);
            if (!it) return nullptr;
            return &it->data;
        }

        const DataType* GetData(HandleType handle) const
        {
            const TableElement* it = m_Pool.GetResource(handle);
            if (!it) return nullptr;
            return &it->data;
        }

        void Clear()
        {
            m_Pool.Clear();
            m_Map.clear();
            m_Temp.clear();
            m_SortedSize = 0;
            m_Buffer.clear(); 
        }

        void Resolve()
        {
            size_t unsorted_count = m_Map.size() - m_SortedSize;
            if (unsorted_count == 0) return;

            m_Temp.resize(unsorted_count);
            RadixSort64(
                std::span(m_Map).subspan(m_SortedSize),
                std::span(m_Temp),
                [](const HashData& item) -> uint64_t { return item.hash_code; }
            );

            if (m_SortedSize > 0)
            {
                std::inplace_merge(m_Map.begin(), m_Map.begin() + m_SortedSize, m_Map.end(), 
                [](const HashData& a, const HashData& b) { return a.hash_code < b.hash_code; });
            }

            m_SortedSize = m_Map.size();
        }

        template<typename Func>
        void ForEach(Func&& func)
        {
            m_Pool.Loop([&func](TableElement& element) {
                func(element.data);
            });
        }

    private:
        std::string_view CalcView(uint32_t offset) const
        {
            if (offset >= m_Buffer.size()) return std::string_view{};
            const char* ptr = m_Buffer.data() + offset;
            uint16_t length = 0;
            std::memcpy(&length, ptr, sizeof(uint16_t));
            ptr += sizeof(uint16_t);
            return std::string_view(ptr, length);
        }

        template<typename... Args>
        HandleType Commit(std::string_view key, uint64_t hash, Args&&... args)
        {
            if (key.size() > std::numeric_limits<uint16_t>::max())
            {
                AE_CORE_ERROR("Table key {0} is too long - {1} characters", key, key.size());
                return HandleType::Null();
            }
            uint32_t offset = static_cast<uint32_t>(m_Buffer.size());
            uint16_t length = static_cast<uint16_t>(key.size());

            m_Buffer.resize(m_Buffer.size() + sizeof(uint16_t) + key.size());
            std::memcpy(m_Buffer.data() + offset, &length, sizeof(uint16_t));
            std::memcpy(m_Buffer.data() + offset + sizeof(uint16_t), key.data(), key.size());

            HandleType handle = m_Pool.CreateResource(offset, std::forward<Args>(args)...);
            m_Map.push_back({ offset, hash, handle });
            return handle;
        }

        HashData* Search(std::string_view key, uint64_t hash)
        {
            auto sorted_end = m_Map.begin() + m_SortedSize;
            auto it = std::lower_bound(m_Map.begin(), sorted_end, hash,
                [](const HashData& entry, uint64_t h) { return entry.hash_code < h; });

            while (it != sorted_end && it->hash_code == hash)
            {
                if (CalcView(it->byte_offset) == key) return std::to_address(it);
                ++it;
            }

            for (size_t i = m_SortedSize; i < m_Map.size(); ++i)
            {
                if (m_Map[i].hash_code == hash && CalcView(m_Map[i].byte_offset) == key)
                    return &m_Map[i];
            }

            return nullptr;
        }

        const HashData* Search(std::string_view key, uint64_t hash) const
        {
            auto sorted_end = m_Map.begin() + m_SortedSize;
            auto it = std::lower_bound(m_Map.begin(), sorted_end, hash,
                [](const HashData& entry, uint64_t h) { return entry.hash_code < h; });

            while (it != sorted_end && it->hash_code == hash)
            {
                if (CalcView(it->byte_offset) == key) return std::to_address(it);
                ++it;
            }

            for (size_t i = m_SortedSize; i < m_Map.size(); ++i)
            {
                if (m_Map[i].hash_code == hash && CalcView(m_Map[i].byte_offset) == key)
                    return &m_Map[i];
            }

            return nullptr;
        }

        size_t m_SortedSize = 0;
        ResourcePool<HandleType, TableElement> m_Pool;
        std::vector<HashData> m_Map;
        std::vector<HashData> m_Temp;
        std::vector<char> m_Buffer;
    };
}