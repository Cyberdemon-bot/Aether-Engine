#pragma once

#include "Aether/Importer/AnimationParser.h"

namespace Aether {

    class GLTF_AnimationParser : public AnimationParser
    {
    public:
        virtual ~GLTF_AnimationParser() = default;
        virtual Ref<RigAnimsCreateInfo> ParseRigAnim(void* data) override;
    
    private:
        void ParseRigs(void* data, Ref<RigAnimsCreateInfo> result);
        void ParseClips(void* data, Ref<RigAnimsCreateInfo> result);
    };

}