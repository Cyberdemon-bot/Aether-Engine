#include "aepch.h"
#include "Aether/Scripting/Promise.h"

namespace Aether {

    void Promise::OnSuccess(const Delegate<void(const ScriptTable&)>& func)
    {
        if (IsFulfilled()) func(m_Result);
        if (!IsSettled()) m_OnFulfilled.push_back(func);
    }
    
    void Promise::OnError(const Delegate<void(const ScriptTable&)>& func)
    {
        if (IsRejected()) func(m_Result);
        if (!IsSettled()) m_OnRejected.push_back(func);
    }
    
    void Promise::Resolve(bool isSuccessed, ScriptTable result)
    {
        if (IsSettled()) return;

        m_Result = result;
        m_State = isSuccessed ? State::Fulfilled : State::Rejected;

        auto container = std::move(isSuccessed ? m_OnFulfilled : m_OnRejected);
        m_OnFulfilled.clear();
        m_OnRejected.clear();

        for (auto& cb : container) cb(m_Result);
    }
}