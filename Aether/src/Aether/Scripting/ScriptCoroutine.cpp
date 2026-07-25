#include "aepch.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Core/JobSystem.h"
#include "Aether/Core/ServiceManager.h"

namespace Aether {
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

        if (jobCount == 0) 
        { 
            MarkCoroutineDone(handle); 
            return; 
        }

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
            else if (task.Type == WaitType::Event)
            {
                task.AwaitEvent = result[0].get<std::string>();
                task.Timer = rc == 3 ? result[1].get<float>() : -1.0f;
            }
            auto handle = m_Coroutines.SaveResource(task);
            auto stored = m_Coroutines.GetResource(handle);
            if (stored) 
            {
                stored->Self = handle;
                Handle<Coroutine> selfHandle = stored->Self;
                if (stored->Type == WaitType::Event)
                {
                    stored->EventCbHandle = m_EventManager.AddNativeListener(
                        stored->AwaitEvent, 
                        [handle, this](const ScriptArgs&) { this->MarkCoroutineDone(handle); } 
                    );
                }
                if (stored->Type == WaitType::Job)
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
        if (task->Type == WaitType::Event && task->EventCbHandle.IsValid()) 
            m_EventManager.RemoveNativeListener(task->EventCbHandle, task->AwaitEvent);
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
                case WaitType::Event:
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
                else result = task.Co();

                if (task.Type == WaitType::Event) 
                {
                    m_EventManager.RemoveNativeListener(task.EventCbHandle, task.AwaitEvent);
                    task.EventCbHandle = {};
                }
                
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
                    else if (task.Type == WaitType::Event) 
                    {
                        task.AwaitEvent = result[0].get<std::string>();
                        task.Timer = rc == 3 ? result[1].get<float>() : -1.0f;
                        Handle<Coroutine> selfHandle = task.Self; 
                        task.EventCbHandle = m_EventManager.AddNativeListener(task.AwaitEvent, 
                        [selfHandle, this](const ScriptArgs&) { this->MarkCoroutineDone(selfHandle); });
                    }
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