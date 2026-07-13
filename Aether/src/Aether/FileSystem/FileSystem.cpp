#include "aepch.h"
#include "Aether/Core/Log.h"
#include "Aether/FileSystem/FileSystem.h"

namespace Aether {

    void FileSystem::Init()
    {
        m_Table.Init();
        m_PathMap.reserve(16);
        m_Mounts.reserve(16);
    }

    void FileSystem::Shutdown()
    {
        m_Table.Loop([](Entry& entry)
        {
            delete[] entry.data.bytes;
        });
        m_Table.Shutdown();
        m_PathMap.clear();
        m_Mounts.clear();
    }

    bool FileSystem::ValidatePath(std::string_view value, std::string_view prefix) const
    {
        if (value.size() < prefix.size()) return false;
        if (value.compare(0, prefix.size(), prefix) != 0) return false;

        if (value.size() == prefix.size()) return true;           
        if (!prefix.empty() && prefix.back() == '/') return true; 
        return value[prefix.size()] == '/';                      
    }

    void FileSystem::Mount(const std::string& virtual_prefix, Ref<FileProvider> provider, int priority)
    {
        std::string fixed_prefix = virtual_prefix;
        if (!fixed_prefix.empty() && fixed_prefix.back() != '/') fixed_prefix += '/';
        m_Mounts.push_back({fixed_prefix, std::move(provider), priority}); 
        std::sort(m_Mounts.begin(), m_Mounts.end(), [](const MountPoint& a, const MountPoint& b)
        {
            return a.priority > b.priority; 
        });
    }

    void FileSystem::Unmount(const std::string& virtual_prefix)
    {
        m_Mounts.erase(
            std::remove_if(m_Mounts.begin(), m_Mounts.end(), [&](const MountPoint& m) 
            { 
                return m.virtual_prefix == virtual_prefix; 
            }),
            m_Mounts.end());
    }

    FileProvider* FileSystem::Resolve(std::string_view virtual_path, std::string_view& out_relative) const
    {
        for (const MountPoint& mount : m_Mounts)
        {   
            if (!ValidatePath(virtual_path, mount.virtual_prefix)) continue;
            std::string_view relative = virtual_path.substr(mount.virtual_prefix.size());
            
            if (!mount.provider->Exists(relative)) continue;
            
            out_relative = relative; 
            return mount.provider.get();
        }
        return nullptr;
    }

    bool FileSystem::Exists(const std::string& virtual_path) const
    {
        std::string_view relative;
        return Resolve(virtual_path, relative) != nullptr;
    }

    Handle<FileData> FileSystem::Open(const std::string& virtual_path)
    {
        auto cached = m_PathMap.find(virtual_path);
        if (cached != m_PathMap.end())
        {
            auto* entry = m_Table.GetResource(cached->second);
            if (entry)
            {
                entry->ref_count++;
                return cached->second;
            }
            m_PathMap.erase(cached);
        }

        std::string_view relative;
        FileProvider* provider = Resolve(virtual_path, relative);
        if (!provider)
        {
            AE_CORE_WARN("FileSystem::Open: no mount resolves '{}'", virtual_path);
            return Handle<FileData>::MakeInvalid();
        }

        FileData data;
        if (!provider->Read(relative, data))
        {
            AE_CORE_ERROR("FileSystem::Open: provider failed to read '{}'", virtual_path);
            return Handle<FileData>::MakeInvalid();
        }

        Entry entry{ data, 1, virtual_path };
        Handle<FileData> handle = m_Table.CreateResource(entry);
        m_PathMap[virtual_path] = handle;
        return handle;
    }

    void FileSystem::Close(Handle<FileData> handle)
    {
        auto* entry = m_Table.GetResource(handle);
        if (!entry) return;

        if (entry->ref_count == 0)
        {
            AE_CORE_WARN("FileSystem::Close: double-close on handle (index {})", handle.index);
            return;
        }

        entry->ref_count--;
        if (entry->ref_count > 0) return;

        m_PathMap.erase(entry->virtual_path);

        delete[] entry->data.bytes;
        entry->data = {};
        m_Table.DestroyResource(handle);
    }

    FileData FileSystem::GetBytes(Handle<FileData> handle)
    {
        Entry* entry = m_Table.GetResource(handle);
        if (!entry) return FileData{}; 
        return entry->data;
    }

    bool FileSystem::IsValid(Handle<FileData> handle)
    {
        return m_Table.GetResource(handle) != nullptr;
    }
}