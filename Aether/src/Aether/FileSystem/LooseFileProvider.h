#pragma once

#include "Aether/FileSystem/FileProvider.h"

namespace Aether {

    class AETHER_API LooseFileProvider : public FileProvider
    {
    public:
        LooseFileProvider(std::string root)
            : m_Root(std::filesystem::weakly_canonical(std::filesystem::absolute(std::filesystem::path(std::move(root)))))
        {}

        virtual bool Exists(std::string_view relative_path) const override;
        virtual bool Read(std::string_view relative_path, FileData& outData) override;
        virtual std::vector<std::string> List(std::string_view dir) const override;
        virtual void Free(FileData& data) override;
    private:
        std::filesystem::path m_Root;
        std::filesystem::path ResolvePath(std::string_view relative_path) const;
    };
}