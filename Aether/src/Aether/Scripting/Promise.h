#pragma once

#include <vector>
#include <optional>
#include "Aether/Container/Handle.h"
#include "Aether/Scripting/ScriptTable.h"
#include "Aether/Core/Delegate.h"

namespace Aether {

    struct AggregateState
    {
        uint32_t total    = 0;
        uint32_t counter  = 0;
        bool rejected = false;
        bool settled  = false;
        std::vector<ScriptTable> results;
    };
    struct Promise
    {
        enum class State { Pending, Fulfilled, Rejected };

        struct Handler
        {
            Delegate<ScriptTable(const ScriptTable&)> OnFulfilled;
            Delegate<ScriptTable(const ScriptTable&)> OnRejected;
            Handle<Promise> NextPromise = Handle<Promise>::MakeInvalid();
        };

        Handle<Promise> SelfHandle = Handle<Promise>::MakeInvalid();

        State CurrentState = State::Pending;
        ScriptTable Result;
        std::vector<Handler> Handlers;
        std::vector<Delegate<void()>> FinallyCallbacks;

        std::optional<AggregateState>   Aggregate;

        bool IsPending() const { return CurrentState == State::Pending;   }
        bool IsFulfilled() const { return CurrentState == State::Fulfilled; }
        bool IsRejected() const { return CurrentState == State::Rejected;  }
        bool IsSettled() const { return CurrentState != State::Pending;   }
    };

} 