#pragma once

#include "Aether/Importer/AnimationParser.h"

namespace Aether {

    class GLTF_AnimationParser : public AnimationParser
    {
    public:
        virtual ~GLTF_AnimationParser() = default;
        virtual Ref<SkelAnimInfo> Parsing(void* data) override;
    
    private:
        void ParseSkels(void* data, Ref<SkelAnimInfo> result);
        void ParseClips(void* data, Ref<SkelAnimInfo> result);
    };

}