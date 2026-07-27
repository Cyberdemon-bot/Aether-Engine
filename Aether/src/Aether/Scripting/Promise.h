#pragma once

#include <vector>
#include "Aether/Container/Handle.h"
#include "Aether/Scripting/ScriptTable.h"
#include "Aether/Core/Delegate.h"

namespace Aether {

    class Promise
    {
    public:
        Promise() = default;

        void OnSuccess(const Delegate<void(const ScriptTable&)>& func);
        void OnError(const Delegate<void(const ScriptTable&)>& func);

        void Resolve(bool isSuccessed, ScriptTable result);

        const ScriptTable& GetResult() const { return m_Result; }
        bool IsFulfilled() const { return m_State == State::Fulfilled; }
        bool IsRejected() const { return m_State == State::Rejected; }
        bool IsSettled() const { return m_State != State::Pending; }
    private:
        enum class State
        {
            Pending, Fulfilled, Rejected
        };

        State m_State = State::Pending;
        ScriptTable m_Result;

        std::vector<Delegate<void(const ScriptTable&)>> m_OnFulfilled;
        std::vector<Delegate<void(const ScriptTable&)>> m_OnRejected;
    };
}