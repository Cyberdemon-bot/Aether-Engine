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
}