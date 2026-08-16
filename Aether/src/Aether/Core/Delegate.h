#pragma once

#include <type_traits>
#include <utility>
#include <cstddef>

namespace Aether {
    template<typename Signature>
    class Delegate;

    template<typename Ret, typename... Args>
    class Delegate<Ret(Args...)>
    {
    public:
        Delegate() = default;
        Delegate(std::nullptr_t) {}

        Delegate(Ret(*func)(Args...))
        {
            InitFromFuncPtr(func);
        }

        template<typename Lambda,
         typename = std::enable_if_t<
             !std::is_same_v<std::decay_t<Lambda>, Delegate> &&
             !std::is_same_v<std::decay_t<Lambda>, Ret(*)(Args...)>
         >>
        Delegate(Lambda&& lambda)
        {
            using RealType = std::decay_t<Lambda>;
            if constexpr (std::is_convertible_v<RealType, Ret(*)(Args...)>)
            {
                Ret(*funcPtr)(Args...) = lambda;
                InitFromFuncPtr(funcPtr);
            }
            else 
            {
                m_Instance = new RealType(std::forward<Lambda>(lambda));
                m_Invoke = [](void* inst, Args... args) -> Ret {
                    return (*static_cast<RealType*>(inst))(std::forward<Args>(args)...);
                };
                m_Destroy = [](void* inst) { delete static_cast<RealType*>(inst); };
                m_Copy = [](void* inst) -> void* { return new RealType(*static_cast<RealType*>(inst)); };
            }
        }

        ~Delegate() { Reset(); }

        Delegate(const Delegate& other)
            : m_Invoke(other.m_Invoke), m_Destroy(other.m_Destroy), m_Copy(other.m_Copy)
        {
            m_Instance = (other.m_Copy && other.m_Instance) ? other.m_Copy(other.m_Instance) : nullptr;
        }

        Delegate& operator=(const Delegate& other)
        {
            if (this == &other) return *this;
            Reset();
            m_Invoke = other.m_Invoke; m_Destroy = other.m_Destroy; m_Copy = other.m_Copy;
            m_Instance = (other.m_Copy && other.m_Instance) ? other.m_Copy(other.m_Instance) : nullptr;
            return *this;
        }

        Delegate(Delegate&& other) noexcept
            : m_Instance(other.m_Instance), m_Invoke(other.m_Invoke),
              m_Destroy(other.m_Destroy), m_Copy(other.m_Copy)
        {
            other.m_Instance = nullptr; other.m_Invoke = nullptr;
            other.m_Destroy = nullptr;  other.m_Copy = nullptr;
        }

        Delegate& operator=(Delegate&& other) noexcept
        {
            if (this == &other) return *this;
            Reset();
            m_Instance = other.m_Instance; m_Invoke = other.m_Invoke;
            m_Destroy = other.m_Destroy;   m_Copy = other.m_Copy;

            other.m_Instance = nullptr; other.m_Invoke = nullptr;
            other.m_Destroy = nullptr;  other.m_Copy = nullptr;
            return *this;
        }

        explicit operator bool() const { return m_Invoke != nullptr; }

        Ret operator()(Args... args) const
        {
            if (m_Invoke) return m_Invoke(m_Instance, std::forward<Args>(args)...);
            if constexpr (!std::is_void_v<Ret>) return Ret{};
        }

    private:
        void InitFromFuncPtr(Ret(*func)(Args...))
        {
            if (!func) return;
            m_Instance = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(func));
            m_Invoke = [](void* inst, Args... args) -> Ret {
                auto fn = reinterpret_cast<Ret(*)(Args...)>(reinterpret_cast<uintptr_t>(inst));
                return fn(std::forward<Args>(args)...);
            };
            m_Destroy = nullptr;
            m_Copy = [](void* inst) -> void* { return inst; };
        }

        void Reset()
        {
            if (m_Destroy && m_Instance)
            {
                m_Destroy(m_Instance);
                m_Instance = nullptr;
            }
        }

    private:
        void* m_Instance = nullptr;
        Ret (*m_Invoke)(void*, Args...) = nullptr;
        void(*m_Destroy)(void*) = nullptr;
        void*(*m_Copy)(void*) = nullptr;
    };
}