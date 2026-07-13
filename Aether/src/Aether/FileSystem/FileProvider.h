#pragma once

#include <string_view>
#include <filesystem>
#include <vector>
#include "Aether/FileSystem/FileData.h"

namespace Aether {
    class FileProvider
    {
    public:
        virtual ~FileProvider() = default;
        virtual bool Exists(std::string_view relative_path) const = 0;
        virtual bool Read(std::string_view relative_path, FileData& out) = 0;
        virtual std::vector<std::string> List(std::string_view dir) const = 0;
    };

    class LooseFileProvider : public FileProvider
    {
    public:
        LooseFileProvider(std::string root)
            : m_Root(std::filesystem::absolute(std::filesystem::path(std::move(root))))
        {}

        virtual bool Exists(std::string_view relative_path) const override;
        virtual bool Read(std::string_view relative_path, FileData& outData) override;
        virtual std::vector<std::string> List(std::string_view dir) const override;
    private:
        std::filesystem::path m_Root;
        std::filesystem::path ResolvePath(std::string_view relative_path) const;
    };
}