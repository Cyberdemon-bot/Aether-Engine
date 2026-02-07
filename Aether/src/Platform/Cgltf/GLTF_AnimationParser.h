#pragma once

#include "Aether/Importer/AnimationParser.h"

namespace Aether {

    class GLTF_AnimationParser : public AnimationParser
    {
    public:
        virtual ~GLTF_AnimationParser() = default;
        virtual Ref<SkelAnimInfo> Parsing(void* data) override;
    
    private:
        Ref<SkelAnimInfo> ParseSkelAnim(void* data);
    };

}