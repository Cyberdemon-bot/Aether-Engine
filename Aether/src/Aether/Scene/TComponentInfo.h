#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Container/Handle.h"
#include <string_view>

namespace Aether {
    struct TComponentInfo // type delc
    {
        std::string_view name = "Invalid";
        Handle<TComponentInfo> id = Handle<TComponentInfo>::Null();
        size_t size, alignment;
        void (*ctor)(void* ptr, size_t count); 
        void (*dtor)(void* ptr, size_t count); 
        void (*move)(void* dst, void* src, size_t count); 

        bool IsRegistered() const { return id.Blend() != Handle<TComponentInfo>::Null().Blend(); }
    };

    template<typename T>
    struct ComponentInfoFactory
    {
        static TComponentInfo Create()
        {
            TComponentInfo info{};
            info.name = type_name<T>();
            info.size = sizeof(T);
            info.alignment = alignof(T);

            if constexpr (!std::is_trivially_default_constructible_v<T>)
            {
                info.ctor = [](void* ptr, size_t count)
                {
                    T* typedPtr = static_cast<T*>(ptr);
                    for (size_t i = 0; i < count; ++i) new(typedPtr + i) T();
                };
            }
            else info.ctor = nullptr;

            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                info.dtor = [](void* ptr, size_t count)
                {
                    T* typedPtr = static_cast<T*>(ptr);
                    for (size_t i = 0; i < count; ++i) typedPtr[i].~T();
                };
            }
            else info.dtor = nullptr;

            if constexpr (!std::is_trivially_copyable_v<T>)
            {
                info.move = [](void* dst, void* src, size_t count)
                {
                    T* typedDst = static_cast<T*>(dst);
                    T* typedSrc = static_cast<T*>(src);
                    for (size_t i = 0; i < count; ++i) typedDst[i] = std::move(typedSrc[i]);
                };
            }
            else info.move = nullptr;

            return info;
        }
    };
}