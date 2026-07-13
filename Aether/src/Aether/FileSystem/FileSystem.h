#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include "Aether/Core/Base.h"
#include "Aether/Container/Handle.h"
#include "Aether/Container/ResourcePool.h"
#include "Aether/FileSystem/FileData.h"
#include "Aether/FileSystem/FileProvider.h"

namespace Aether {

    struct Entry
    {
        FileData data = {};
        uint32_t ref_count = 0;
        std::string virtual_path = "";
    };

    struct MountPoint
    {
        std::string virtual_prefix;
        Ref<FileProvider> provider;
        int priority = 0;
    };

    class AETHER_API FileSystem
    {
    public:
        void Init();
        void Shutdown();

        Handle<FileData> Open(const std::string& virtual_path);
        void Close(Handle<FileData> handle);
        FileData GetBytes(Handle<FileData> handle);
        bool IsValid(Handle<FileData> handle);
        bool Exists(const std::string& virtual_path) const;

        void Mount(const std::string& virtual_prefix, Ref<FileProvider> provider, int priority = 0);
        void Unmount(const std::string& virtual_prefix);
    private:
        bool ValidatePath(std::string_view value, std::string_view prefix) const;
        FileProvider* Resolve(std::string_view virtual_path, std::string_view& out_relative) const;

        ResourcePool<Handle<FileData>, Entry> m_Table;
        std::unordered_map<std::string, Handle<FileData>> m_PathMap;
        std::vector<MountPoint> m_Mounts;
    };
}