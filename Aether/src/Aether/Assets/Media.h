#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"

namespace Aether {

    class Resource;
    struct Bytecode;
    struct AudioSource; 

    struct Image : public Asset
    {
        Image(Handle<Resource> handle)
        {
            m_Handle = handle;
        }
        Handle<Resource> m_Handle;
    };

    struct Audio : public Asset
    {
        Audio(Handle<AudioSource> handle)
        {
            m_Handle = handle;
        }
        Handle<AudioSource> m_Handle;
    };

    struct Script : public Asset
    {
        Script(Handle<Bytecode> handle)
        {
            m_Handle = handle;
        }
        Handle<Bytecode> m_Handle;
    };
}