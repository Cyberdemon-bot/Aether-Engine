#pragma once

namespace Aether {
    class TypeRegistry
    {
    public:
        template<typename T>
        static uint32_t GetCode()
        {
            static const uint32_t code = s_cnt++;
            return code;
        }
    private:
        inline static uint32_t s_cnt = 0;
    };
}