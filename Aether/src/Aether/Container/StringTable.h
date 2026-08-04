#pragma once

#include <vector>
#include <string_view>
#include "Aether/Container/Handle.h"
#include "Aether/Container/ResourcePool.h"

namespace Aether {

    struct StringData;

    struct HashData
    {
        Handle<StringData> handle;
        uint64_t hash_code;
    };

    class StringTable
    {
    public:
        void Init();
        void Shutdown();
        
        Handle<StringData> Search(std::string_view key) const;
        Handle<StringData> Get(std::string_view key);

        std::string_view GetView(Handle<StringData> handle) const;
        std::string GetString(Handle<StringData> handle) const;

        void Resolve();
    private:
        Handle<StringData> Commit(std::string_view key, uint64_t hash);
        Handle<StringData> Search(std::string_view key, uint64_t hash) const;

        ResourcePool<Handle<StringData>, uint32_t> m_Pool;
        std::vector<HashData> m_Map;   
        std::vector<HashData> m_Queue;  
        std::vector<char> m_Buffer;
    };
}