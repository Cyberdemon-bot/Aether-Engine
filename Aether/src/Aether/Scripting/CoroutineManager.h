#pragma once

#include <vector>
#include <sol/sol.hpp>
#include "Aether/Core/Timestep.h"
#include "Aether/Container/Handle.h"
#include "Aether/Container/ResourcePool.h"
#include "Aether/Scripting/ScriptTable.h"

namespace Aether {

    struct ScriptInstance;

    enum class CoroutineState
    {
        Running, Suspended, Dead
    };

    enum class WaitType
    {
        None, Seconds, Manual
    };

    struct CoroutineTask
    {
        sol::thread thread;
        sol::main_reference ref;
        sol::coroutine co;
        Handle<ScriptInstance> owner;
        Handle<CoroutineTask> self_handle;
        CoroutineState state = CoroutineState::Suspended;
        WaitType condition = WaitType::None;
        float timer = 0.0f;

        bool IsValid()
        {
            return co.valid();
        }
    };

    struct ResumeRequest
    {
        Handle<CoroutineTask> handle;
        std::vector<sol::object> results;
    };

    class CoroutineManager
    {
    public:
        CoroutineManager() = default;

        void Init(sol::state_view lua);
        void Shutdown();

        Handle<CoroutineTask> StartCoroutine(sol::function func, Handle<ScriptInstance> owner);
        void YieldTask(Handle<CoroutineTask> handle);
        void WaitForSeconds(Handle<CoroutineTask> handle, float seconds);
        void WaitForManual(Handle<CoroutineTask> handle);

        void StopCoroutine(Handle<CoroutineTask> handle);
        void StopAllCoroutines(Handle<ScriptInstance> owner);

        void ResumeTask(Handle<CoroutineTask> handle, sol::object result = sol::lua_nil);
        void ResumeTask(Handle<CoroutineTask> handle, std::vector<sol::object> results);

        void Update(Timestep ts);
        void Clear();

        Handle<CoroutineTask> GetCurrentRunningTask(sol::thread target);

        void SetRecursionDepth(uint32_t depth);
    private:
        void MarkForStop(Handle<CoroutineTask> handle);
        std::optional<sol::state_view> m_LuaState;
        ResourcePool<Handle<CoroutineTask>, CoroutineTask> m_Tasks;
        std::vector<Handle<CoroutineTask>> m_StopQueue;
        std::vector<ResumeRequest> m_ResumeQueue;
        std::vector<ResumeRequest> m_NextResumeQueue;

        uint32_t m_RecursionDepth = 3;

        bool ExecuteResume(Handle<CoroutineTask> handle, std::vector<sol::object> results);
    };
}