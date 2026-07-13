#include "aepch.h"
#include "Aether/Core/Log.h"
#include "Aether/FileSystem/FileProvider.h"

namespace Aether {
    bool LooseFileProvider::Exists(std::string_view relative_path) const
    {
        std::error_code ec;
        std::filesystem::path full = ResolvePath(relative_path);

        if (full.empty()) return false;
        return std::filesystem::exists(full, ec) && !std::filesystem::is_directory(full, ec);
    }

    bool LooseFileProvider::Read(std::string_view relative_path, FileData& out)
    {
        std::filesystem::path full = ResolvePath(relative_path);
        if (full.empty())
        {
            AE_CORE_ERROR("LooseFileProvider: path escapes root: {}", relative_path);
            return false;
        }

        std::ifstream file(full, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            AE_CORE_ERROR("LooseFileProvider: failed to open {}", full.string());
            return false;
        }

        std::streamsize size = file.tellg();
        if (size < 0)
        {
            AE_CORE_ERROR("LooseFileProvider: tellg failed for {}", full.string());
            return false;
        }
        file.seekg(0, std::ios::beg);

        uint8_t* buffer = new uint8_t[static_cast<size_t>(size)];
        if (size > 0 && !file.read(reinterpret_cast<char*>(buffer), size))
        {
            AE_CORE_ERROR("LooseFileProvider: read failed for {}", full.string());
            delete[] buffer;
            return false;
        }

        out.bytes = buffer;
        out.size = static_cast<size_t>(size);
        return true;
    }   

    std::vector<std::string> LooseFileProvider::List(std::string_view dir) const
    {
        std::vector<std::string> results;
        std::filesystem::path full = ResolvePath(dir);
        if (full.empty() || !std::filesystem::exists(full)) return results;

        for (auto& entry : std::filesystem::recursive_directory_iterator(full))
        {
            if (entry.is_directory()) continue;

            std::filesystem::path rel = std::filesystem::relative(entry.path(), m_Root);
            results.push_back(rel.generic_string());
        }
        return results;
    }

    std::filesystem::path LooseFileProvider::ResolvePath(std::string_view relative_path) const
    {
        std::filesystem::path joined = m_Root / relative_path;
        std::error_code ec;
        std::filesystem::path canonical = std::filesystem::weakly_canonical(joined, ec);
        if (ec) return {};

        auto rootStr = m_Root.generic_string();
        auto candStr = canonical.generic_string();

        if (candStr.size() < rootStr.size() || candStr.compare(0, rootStr.size(), rootStr) != 0) return {};
        return canonical;
    }
}