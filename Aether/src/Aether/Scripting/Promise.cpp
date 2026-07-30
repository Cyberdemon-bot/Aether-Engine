#include "aepch.h"
#include "Aether/Scripting/Promise.h"
#include "Aether/Scripting/PromiseManager.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Core/ServiceManager.h"

namespace Aether {

    Handle<Promise> Promise::Then(const Delegate<ScriptTable(const ScriptTable&)>& onFulfilled,
                                const Delegate<ScriptTable(const ScriptTable&)>& onRejected)
    {
        auto* script_engine = ServiceManager::GetService<ScriptEngine>();
        Handle<Promise> next = script_engine->CreatePromise();

        Promise* self = script_engine->GetPromise(m_SelfHandle);
        if (self == nullptr) return next;

        Handler handler;
        handler.OnFulfilled = onFulfilled;
        handler.OnRejected = onRejected;
        handler.NextPromise = next;
        self->m_Handlers.push_back(handler);
        if (self->IsSettled()) self->QueueDispatch();

        return next;
    }

    Handle<Promise> Promise::Catch(const Delegate<ScriptTable(const ScriptTable&)>& onRejected)
    {
        return Then(nullptr, onRejected);
    }

    void Promise::Finally(const Delegate<void()>& onFinally)
    {
        if (!onFinally) return;

        m_OnFinally.push_back(onFinally);
        if (IsSettled()) QueueDispatch();
    }

    void Promise::Resolve(ScriptTable result)
    {
        Settle(true, result);
    }

    void Promise::Reject(ScriptTable error)
    {
        Settle(false, error);
    }

    void Promise::Settle(bool isSuccessed, ScriptTable result)
    {
        if (IsSettled()) return;
        m_State = isSuccessed ? State::Fulfilled : State::Rejected;
        m_Result = result;

        QueueDispatch();
    }

    void Promise::QueueDispatch()
    {
        auto* script_engine = ServiceManager::GetService<ScriptEngine>();
        script_engine->GetPromiseManager().QueueSettle(m_SelfHandle);
    }
}