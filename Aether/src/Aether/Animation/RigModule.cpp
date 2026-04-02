#include "Platform/Ozz/Ozz_RigModule.h"

namespace Aether {

    Ref<RigModule> RigModule::Create()
    {
        return CreateRef<Ozz_RigModule>();
    }

}