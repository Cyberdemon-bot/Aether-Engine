#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Scripting/ScriptGlue.h"
#include "Aether/Core/Log.h"
#include "Aether/Core/KeyCodes.h"

namespace Aether {
    sol::meta_function ScriptEngine::OpNameToMeta(std::string_view name)
    {
        if (name == "ADD") return sol::meta_function::addition;
        if (name == "SUB") return sol::meta_function::subtraction;
        if (name == "MUL") return sol::meta_function::multiplication;
        if (name == "DIV") return sol::meta_function::division;
        if (name == "EQ")  return sol::meta_function::equal_to;
        if (name == "LT")  return sol::meta_function::less_than;
        if (name == "LE")  return sol::meta_function::less_than_or_equal_to;
        if (name == "MOD") return sol::meta_function::modulus;
        if (name == "UNM") return sol::meta_function::unary_minus;
        AE_CORE_WARN("[ScriptEngine] Unknown op name: {0}", name);
        return sol::meta_function::addition;
    }

    uint32_t ScriptEngine::Get_MinState()
    {
        auto& instance = GetInstance();
        uint32_t m = instance.StatePool[0].env_count;
        uint32_t result = 0;
        for(size_t i = 1; i < SYS_THREAD_NUM; i++) 
        {
            if (instance.StatePool[i].env_count < m)
            {
                m = instance.StatePool[i].env_count;
                result = i;
            }
        }
        return result;
    }

    ScriptEngine& ScriptEngine::GetInstance()
    {
        static ScriptEngine instance;
        return instance;
    }

    void ScriptEngine::Init()
    {   
       auto& instance = GetInstance();
       instance.StatePool = CreateScope<LuaWorker[]>(SYS_THREAD_NUM);
       LUA_LOOP(lua.open_libraries(sol::lib::base, sol::lib::math);)
       instance.m_Instances.Init();
       RegisterTypes();
       AE_CORE_INFO("ScriptEngine initialized");
    }

    void ScriptEngine::Shutdown()
    {
        auto& instance = GetInstance();
        instance.m_Instances.Shutdown();
    }

    void ScriptEngine::RegisterTypes()
    {
        BindEnum<Key::KeyCode>("Key");
        BindEnum<Mouse::MouseCode>("Mouse");
        BindType<Vec3Binding>("Math");
        BindType<QuatBinding>("Math");
        BindType<TransformComponentBinding>();
        BindModule<MathBinding>("Math");
        BindModule<InputBinding>("Input"); 
    }

    Handle<ScriptTag> ScriptEngine::CreateInstance(Scene* scene, Entity entity)
    {
        auto& instance = GetInstance(); 

        uint32_t idx = Get_MinState();
        auto env_handle = instance.StatePool[idx].CreateEnvironment();
        sol::environment env = *instance.StatePool[idx].env_pool.GetResource(env_handle);
        sol::table self = instance.StatePool[idx].lua.create_table();
        self["Transform"] = &scene->GetComponent<TransformComponent>(entity);
        env["self"] = self;

        auto handle = instance.m_Instances.CreateResource();
        auto slot = instance.m_Instances.GetResource(handle);

        slot->env_hanle = env_handle;
        slot->state_idx = idx;
        return handle; 
    }

    void ScriptEngine::StartInstance(Handle<ScriptTag> handle)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        CallMethod(*slot, "OnStart");
    }

    void ScriptEngine::LoadScript(Handle<ScriptTag> handle, const std::string& path)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;

        auto& lua = instance.StatePool[slot->state_idx].lua;
        auto env = instance.StatePool[slot->state_idx].env_pool.GetResource(slot->env_hanle);
        if (env == nullptr) return;

        auto result = lua.script_file(path, *env);
        if (!result.valid()) 
        {
            sol::error err = result;
            slot->has_error = true;
            AE_CORE_ERROR("[Lua Error] {0}", err.what());
            return;
        }
    }

    void ScriptEngine::DestroyInstance(Handle<ScriptTag> handle)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        CallMethod(*slot, "OnDestroy");

        instance.StatePool[slot->state_idx].RemoveEnvironment(slot->env_hanle);
        instance.m_Instances.DestroyResource(handle);
    }

    void ScriptEngine::UpdateInstance(Handle<ScriptTag> handle, Timestep ts)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        if (slot->has_error) return;

        CallMethod(*slot, "OnUpdate", (float)ts);
    }

    void ScriptEngine::SetActiveStage(Handle<ScriptTag> handle, bool active)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        slot->is_active = active;
    }

    bool ScriptEngine::GetActiveStage(Handle<ScriptTag> handle)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return false;
        return slot->is_active;
    }
    
    void ScriptEngine::Update()
    {

    }
}