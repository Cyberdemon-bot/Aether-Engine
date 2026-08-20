#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"

namespace Aether {

    class Resource;
    struct Bytecode;
    struct AudioSource; 

    struct AImage : public Asset
    {
        AImage(Handle<Resource> handle)
        {
            m_Handle = handle;
        }
        Handle<Resource> m_Handle;
    };

    struct AAudio : public Asset
    {
        AAudio(Handle<AudioSource> handle)
        {
            m_Handle = handle;
        }
        Handle<AudioSource> m_Handle;
    };

    struct AScript : public Asset
    {
        AScript(Handle<Bytecode> handle)
        {
            m_Handle = handle;
        }
        Handle<Bytecode> m_Handle;
    };
}