#pragma once

#include <vector>
#include "Aether/Container/Handle.h"
#include "Aether/Scripting/ScriptTable.h"
#include "Aether/Core/Delegate.h"

namespace Aether {

    class PromiseManager;

    class Promise
    {
    public:
        Promise() = default;

        Handle<Promise> Then(const Delegate<ScriptTable(const ScriptTable&)>& onFulfilled,
                            const Delegate<ScriptTable(const ScriptTable&)>& onRejected = {});
        Handle<Promise> Catch(const Delegate<ScriptTable(const ScriptTable&)>& onRejected);
        void Finally(const Delegate<void()>& onFinally);

        void Resolve(ScriptTable result);
        void Reject(ScriptTable error);
        void Settle(bool isSuccessed, ScriptTable result);

        const ScriptTable& GetResult() const { return m_Result; }
        bool IsFulfilled() const { return m_State == State::Fulfilled; }
        bool IsRejected() const { return m_State == State::Rejected; }
        bool IsSettled() const { return m_State != State::Pending; }

    private:
        enum class State 
        { 
            Pending, Fulfilled, Rejected 
        };

        struct Handler
        {
            Delegate<ScriptTable(const ScriptTable&)> OnFulfilled;
            Delegate<ScriptTable(const ScriptTable&)> OnRejected;
            Handle<Promise> NextPromise = Handle<Promise>::MakeInvalid();
        };
        
        void QueueDispatch();

        Handle<Promise> m_SelfHandle = Handle<Promise>::MakeInvalid();
        State m_State = State::Pending;
        bool m_IsChained = false;
        ScriptTable m_Result;

        std::vector<Handler> m_Handlers;
        std::vector<Delegate<void()>> m_OnFinally;

        friend class PromiseManager;
    };
}