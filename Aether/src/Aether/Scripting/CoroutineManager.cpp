#include "aepch.h"
#include "Aether/Core/Log.h"
#include "Aether/Scripting/CoroutineManager.h"

namespace Aether {
    void CoroutineManager::Init(sol::state_view lua)
    {
        m_LuaState = lua;
        m_Tasks.Init();
        m_StopQueue.reserve(32);
        m_ResumeQueue.reserve(32);
    }

    void CoroutineManager::Shutdown()
    {
        m_Tasks.Shutdown();
        m_StopQueue.clear();
        m_ResumeQueue.clear();
    }

    Handle<CoroutineTask> CoroutineManager::StartCoroutine(sol::function func, Handle<ScriptInstance> owner)
    {
        if (!func.valid()) return Handle<CoroutineTask>::MakeInvalid();

        auto handle = m_Tasks.CreateResource();
        auto* task = m_Tasks.GetResource(handle);
        if (task == nullptr) return Handle<CoroutineTask>::MakeInvalid();

        task->thread = sol::thread::create(m_LuaState);
        task->ref = task->thread;
        sol::state_view state = task->thread.state();
        task->co = sol::coroutine(state, func);
        task->owner = owner;
        task->self_handle = handle;
        task->state = CoroutineState::Suspended;

        ResumeTask(handle);

        return handle;
    }

    bool CoroutineManager::ResumeTask(Handle<CoroutineTask> handle, sol::object result)
    {
        return ResumeTask(handle, std::vector<sol::object>{ result });
    }

    bool CoroutineManager::ResumeTask(Handle<CoroutineTask> handle, std::vector<sol::object> results)
    {
        auto* task = m_Tasks.GetResource(handle);
        if (task == nullptr || task->state == CoroutineState::Dead) return false;
        if (!task->co.valid())
        {
            MarkForStop(handle);
            return false;
        }
 
        task->state = CoroutineState::Running;
        task->condition = WaitType::None;
        sol::coroutine co = task->co;
 
        auto res = co(sol::as_args(results));
 
        task = m_Tasks.GetResource(handle);
        if (task == nullptr || task->state == CoroutineState::Dead) return false;
 
        if (!res.valid())
        {
            sol::error err = res;
            AE_CORE_ERROR("[Coroutine] Task {0} got execution error {1}", handle.Blend(), err.what());
            MarkForStop(handle);
            return false;
        }
 
        if (co.status() == sol::call_status::yielded)
        {
            task->state = CoroutineState::Suspended;
            return true;
        }
 
        MarkForStop(handle);
        return false;
    }

    void CoroutineManager::YieldTask(Handle<CoroutineTask> handle)
    {
        auto* task = m_Tasks.GetResource(handle);
        if (task == nullptr || task->state == CoroutineState::Dead) return;
        task->condition = WaitType::None;
    }

    void CoroutineManager::WaitForSeconds(Handle<CoroutineTask> handle, float seconds)
    {
        auto* task = m_Tasks.GetResource(handle);
        if (task == nullptr || task->state == CoroutineState::Dead) return;
        task->condition = WaitType::Seconds;
        task->timer = seconds;
    }

    void CoroutineManager::WaitForManual(Handle<CoroutineTask> handle)
    {
        auto* task = m_Tasks.GetResource(handle);
        if (task == nullptr || task->state == CoroutineState::Dead) return;
        task->condition = WaitType::Manual;
    }

    void CoroutineManager::Update(Timestep ts)
    {
        m_ResumeQueue.clear();
        m_Tasks.Loop([ts, this](CoroutineTask& task)
        {
            if (task.state == CoroutineState::Dead) return; 

            if (task.condition == WaitType::Seconds)
            {
                task.timer -= ts.GetSeconds();
                if (task.timer <= 0.0f)
                    m_ResumeQueue.push_back(task.self_handle);
            }
            else if (task.condition == WaitType::None)
            {
                m_ResumeQueue.push_back(task.self_handle);
            }
        });

        for (auto& handle : m_ResumeQueue)
        {
            auto* task = m_Tasks.GetResource(handle);
            if (task && task->state != CoroutineState::Dead) ResumeTask(handle);
        }

        for (auto& handle : m_StopQueue) m_Tasks.DestroyResource(handle);
        m_StopQueue.clear();
    }

    void CoroutineManager::StopCoroutine(Handle<CoroutineTask> handle)
    {
        MarkForStop(handle);
    }

    void CoroutineManager::StopAllCoroutines(Handle<ScriptInstance> owner)
    {
        m_Tasks.Loop([this, owner](CoroutineTask& task)
        {
            if (task.state != CoroutineState::Dead && task.owner.Blend() == owner.Blend())
                MarkForStop(task.self_handle);
        });
    }

    void CoroutineManager::Clear()
    {
        m_Tasks.Clear();
        m_StopQueue.clear();
        m_ResumeQueue.clear();
    }

    Handle<CoroutineTask> CoroutineManager::GetCurrentRunningTask(sol::thread target)
    {
        Handle<CoroutineTask> found = Handle<CoroutineTask>::MakeInvalid();
        m_Tasks.Loop([&target, &found](CoroutineTask& task)
        {
            if (task.thread == target && task.state == CoroutineState::Running) found = task.self_handle;
        });
        return found;
    }

    void CoroutineManager::MarkForStop(Handle<CoroutineTask> handle)
    {
        auto* task = m_Tasks.GetResource(handle);
        if (task == nullptr || task->state == CoroutineState::Dead) return; 
        task->state = CoroutineState::Dead;
        task->ref.reset(); 
        task->co = sol::coroutine(); 
        task->thread = sol::thread();
        m_StopQueue.push_back(handle);
    }
}