#include "Platform/Ozz/Ozz_RigModule.h"

namespace Aether {

    Scope<RigModule> RigModule::Create()
    {
        return CreateScope<Ozz_RigModule>();
    }

}