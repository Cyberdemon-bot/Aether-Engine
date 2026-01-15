#pragma once

#include "LegacyBase.h"
#include <vector>
#include <string>

namespace Aether::Legacy {

    class AETHER_API TextureCube
    {
    private:
        unsigned int m_RendererID;
    public:
        TextureCube(const std::string& path);
        ~TextureCube();

        void Bind(unsigned int slot = 0) const;
        void Unbind() const;

        inline int GetID() const { return m_RendererID; }
    };
}