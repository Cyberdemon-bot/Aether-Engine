#include "aepch.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Core/JobSystem.h"
#include "Aether/Core/ServiceManager.h"

namespace Aether {

    sol::table ScriptEngine::ArgsToTable(const ScriptArgs& args)
    {
        ScriptValueList list = args.GetArgs();
        sol::table t = LuaState.lua.create_table((int)list.size, 0);
        for (uint32_t i = 0; i < list.size; i++)
            t[i + 1] = ToSolObject(LuaState.lua, list.data[i]);
        return t;
    }

    void ScriptEngine::MarkCoroutineDone(Handle<Coroutine> handle)
    {
        auto task = m_Coroutines.GetResource(handle);
        if (task) task->DoneFlag.store(true, std::memory_order_release);
    }

    void ScriptEngine::DispatchJobBatch(Handle<Coroutine> handle, sol::protected_function_result& result, int jobCount)
    {
        auto task = m_Coroutines.GetResource(handle);
        if (!task) return;

        task->JobResults.assign(jobCount, ScriptValue{});
        task->PendingJobs.store(jobCount, std::memory_order_relaxed);
        task->Timer = -1.0f;

        if (jobCount == 0) { MarkCoroutineDone(handle); return; }

        for (int i = 0; i < jobCount; i++)
        {
            sol::table jobSpec = result[i];
            std::string funcName = jobSpec[1].get<std::string>();
            size_t tblSize = jobSpec.size();

            ScriptArgs args;
            for (size_t j = 2; j <= tblSize; j++)
                args.Pushback(FromSolObject(jobSpec[j]));

            auto it = std::find_if(m_NativeFuncs.begin(), m_NativeFuncs.end(),
                [&funcName](const NativeFunc& f) { return f.name == funcName; });

            if (it != m_NativeFuncs.end())
            {
                auto nativeFunc = it->native;
                int jobIndex = i;
                ServiceManager::GetService<JobSystem>()->SubmitJob([handle, args, nativeFunc, jobIndex, this]() mutable
                {
                    ScriptValue ret = nativeFunc(args);
                    auto t = this->m_Coroutines.GetResource(handle);
                    if (t)
                    {
                        t->JobResults[jobIndex] = ret;
                        if (t->PendingJobs.fetch_sub(1, std::memory_order_acq_rel) == 1)
                            this->MarkCoroutineDone(handle);
                    }
                });
            }
            else
            {
                AE_CORE_ERROR("[Script] WaitJob: native function '{0}' not found", funcName);
                if (task->PendingJobs.fetch_sub(1, std::memory_order_acq_rel) == 1)
                    MarkCoroutineDone(handle);
            }
        }
    }

    void ScriptEngine::RegisterEventWait(Handle<Coroutine> handle, sol::protected_function_result& result, int rc, bool isRace)
    {
        auto task = m_Coroutines.GetResource(handle);
        if (!task) return;

        sol::table events = result[0];
        int eventCount = (int)events.size();
        task->Timer = (rc == 3) ? result[1].get<float>() : -1.0f;

        task->AwaitEvents.assign(eventCount, {});
        task->EventCbHandles.assign(eventCount, {});

        if (eventCount == 0) { MarkCoroutineDone(handle); return; }

        if (isRace)
            task->FiredEventIndex.store(-1, std::memory_order_relaxed);
        else
        {
            task->PendingEvents.store(eventCount, std::memory_order_relaxed);
            task->EventResults.assign(eventCount, ScriptArgs{});
            task->EventFired.assign(eventCount, false);
        }

        for (int i = 0; i < eventCount; i++)
        {
            task->AwaitEvents[i] = events[i + 1].get<std::string>();

            if (isRace)
            {
                task->EventCbHandles[i] = m_EventManager.AddNativeListener(
                    task->AwaitEvents[i],
                    [handle, i, this](const ScriptArgs& args)
                    {
                        auto t = this->m_Coroutines.GetResource(handle);
                        if (!t) return;

                        int expected = -1;
                        if (t->FiredEventIndex.compare_exchange_strong(expected, i, std::memory_order_acq_rel))
                        {
                            t->FiredEventArgs = args;
                            this->MarkCoroutineDone(handle);
                        }
                    }
                );
            }
            else
            {
                task->EventCbHandles[i] = m_EventManager.AddNativeListener(
                    task->AwaitEvents[i],
                    [handle, i, this](const ScriptArgs& args)
                    {
                        auto t = this->m_Coroutines.GetResource(handle);
                        if (!t) return;

                        if (t->EventFired[i]) return;      
                        t->EventFired[i] = true;           


                        t->EventResults[i] = args;
                        if (t->PendingEvents.fetch_sub(1, std::memory_order_acq_rel) == 1)
                            this->MarkCoroutineDone(handle);
                    }
                );
            }
        }
    }

    Handle<Coroutine> ScriptEngine::StartCoroutineAPI(Handle<ScriptInstance> owner, sol::function func)
    {
        sol::thread runner = sol::thread::create(LuaState.lua.lua_state());
        sol::coroutine co = sol::coroutine(runner.state(), func);
        auto result = co();
        if (!result.valid())
        {
            sol::error err = result;
            AE_CORE_ERROR("[Script] Coroutine failed to start: {0}", err.what());
            return {};
        }

        if (co.runnable())
        {
            int rc = result.return_count();
            CoroutineTask task;
            task.Owner = owner;
            task.Runner = std::move(runner);
            task.Co = co;
            task.Type = (WaitType)result[rc - 1].get<int>();
            if (task.Type == WaitType::Time) task.Timer = result[0].get<float>();
            else if (task.Type == WaitType::Frame) task.Frames = result[0].get<int>();
            auto handle = m_Coroutines.SaveResource(task);
            auto stored = m_Coroutines.GetResource(handle);
            if (stored)
            {
                stored->Self = handle;
                Handle<Coroutine> selfHandle = stored->Self;
                if (stored->Type == WaitType::EventAny) RegisterEventWait(selfHandle, result, rc, true);
                else if (stored->Type == WaitType::EventAll) RegisterEventWait(selfHandle, result, rc, false);
                else if (stored->Type == WaitType::Job)
                {
                    int jobCount = rc - 1;
                    DispatchJobBatch(selfHandle, result, jobCount);
                }
            }
            return handle;
        }

        return Handle<Coroutine>::MakeInvalid();
    }

    void ScriptEngine::KillCoroutineAPI(Handle<Coroutine> handle)
    {
        auto task = m_Coroutines.GetResource(handle);
        if (!task) return;

        if (task->Type == WaitType::EventAny || task->Type == WaitType::EventAll)
        {
            for (size_t i = 0; i < task->AwaitEvents.size(); i++)
                m_EventManager.RemoveNativeListener(task->EventCbHandles[i], task->AwaitEvents[i]);
        }
        m_Coroutines.DestroyResource(handle);
    }

    void ScriptEngine::UpdateCoroutines(Timestep ts)
    {
        m_Coroutines.Loop([&, this](CoroutineTask& task)
        {
            if (!m_Instances.GetResource(task.Owner))
            {
                KillCoroutineAPI(task.Self);
                return;
            }

            bool shouldResume = false;
            switch (task.Type)
            {
                case WaitType::Time:
                    task.Timer -= (float)ts;
                    if (task.Timer <= 0.0f) shouldResume = true;
                    break;
                case WaitType::Frame:
                    task.Frames--;
                    if (task.Frames <= 0) shouldResume = true;
                    break;
                case WaitType::Job:
                case WaitType::EventAny:
                case WaitType::EventAll:
                    if (task.DoneFlag.load(std::memory_order_acquire)) shouldResume = true;
                    else if (task.Timer >= 0.0f)
                    {
                        task.Timer -= (float)ts;
                        if (task.Timer <= 0.0f) shouldResume = true;
                    }
                    break;
                default: shouldResume = true; break;
            }

            if (shouldResume)
            {
                task.DoneFlag.store(false);
                sol::protected_function_result result;

                if (task.Type == WaitType::Job)
                {
                    sol::table resultsTable = LuaState.lua.create_table(int(task.JobResults.size()), 0);
                    for (size_t i = 0; i < task.JobResults.size(); i++)
                        resultsTable[i + 1] = ToSolObject(LuaState.lua, task.JobResults[i]);
                    result = task.Co(resultsTable);
                    task.JobResults.clear();
                }
                else if (task.Type == WaitType::EventAny)
                {
                    for (size_t i = 0; i < task.AwaitEvents.size(); i++)
                        m_EventManager.RemoveNativeListener(task.EventCbHandles[i], task.AwaitEvents[i]);

                    int winner = task.FiredEventIndex.load(std::memory_order_acquire);
                    if (winner >= 0)
                        result = task.Co(task.AwaitEvents[winner], ArgsToTable(task.FiredEventArgs));
                    else
                        result = task.Co(sol::lua_nil, sol::lua_nil);

                    task.AwaitEvents.clear();
                    task.EventCbHandles.clear();
                    task.FiredEventIndex.store(-1, std::memory_order_relaxed);
                }
                else if (task.Type == WaitType::EventAll)
                {
                    for (size_t i = 0; i < task.AwaitEvents.size(); i++)
                        m_EventManager.RemoveNativeListener(task.EventCbHandles[i], task.AwaitEvents[i]);

                    sol::table resultsTable = LuaState.lua.create_table(int(task.EventResults.size()), 0);
                    for (size_t i = 0; i < task.EventResults.size(); i++)
                        resultsTable[i + 1] = ArgsToTable(task.EventResults[i]);

                    result = task.Co(resultsTable);
                    task.AwaitEvents.clear();
                    task.EventCbHandles.clear();
                    task.EventResults.clear();
                }
                else result = task.Co();

                if (!result.valid())
                {
                    sol::error err = result;
                    AE_CORE_ERROR("[Script] Coroutine error: {0}", err.what());
                    KillCoroutineAPI(task.Self);
                    return;
                }
                int rc = result.return_count();

                if (task.Co.runnable())
                {
                    task.Type = (WaitType)result[rc - 1].get<int>();
                    if (task.Type == WaitType::Time) task.Timer = result[0].get<float>();
                    else if (task.Type == WaitType::Frame) task.Frames = result[0].get<int>();
                    else if (task.Type == WaitType::EventAny) RegisterEventWait(task.Self, result, rc, true);
                    else if (task.Type == WaitType::EventAll) RegisterEventWait(task.Self, result, rc, false);
                    else if (task.Type == WaitType::Job)
                    {
                        int jobCount = rc - 1;
                        DispatchJobBatch(task.Self, result, jobCount);
                    }
                }
                else KillCoroutineAPI(task.Self);
            }
        });
    }
}