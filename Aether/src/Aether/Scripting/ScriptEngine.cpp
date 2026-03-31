#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Scripting/ScriptGlue.h"
#include "Aether/Core/Log.h"

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

    ScriptEngine& ScriptEngine::GetInstance()
    {
        static ScriptEngine instance;
        return instance;
    }

    void ScriptEngine::Init()
    {   
       auto& instance = GetInstance();
       instance.s_LuaState.open_libraries(sol::lib::base, sol::lib::math);
       instance.m_Instances.reserve(100);
       RegisterTypes();
       AE_CORE_INFO("ScriptEngine initialized");
    }

    void ScriptEngine::Shutdown()
    {
        auto& instance = GetInstance();
        instance.m_Instances.clear();
    }

    void ScriptEngine::RegisterTypes()
    {
        auto& instance = GetInstance();
        instance.BindType<Vec3Binding>("Math");
        instance.BindType<QuatBinding>("Math");
        instance.BindType<TransformComponentBinding>();
        instance.BindModule<MathBinding>("Math");
    }

    InstanceHandle ScriptEngine::CreateInstance(Scene* scene, Entity entity)
    {
        auto& instance = GetInstance(); int index;

        if (!instance.FreeList.empty())
        {
            index = instance.FreeList.back();
            instance.FreeList.pop_back();
        }
        else
        {
            index = instance.m_Instances.size();
            instance.m_Instances.emplace_back();
        }

        InstanceHandle handle;
        InstanceSlot& slot = instance.m_Instances[index];
        sol::environment env(instance.s_LuaState, sol::create, instance.s_LuaState.globals());
        sol::table self = instance.s_LuaState.create_table();
        self["Transform"] = &scene->GetComponent<TransformComponent>(entity);
        env["self"] = self;

        slot.env = env;
        handle.index = index;
        handle.generation = slot.generation;

        return handle; 
    }

    void ScriptEngine::StartInstance(InstanceHandle handle)
    {
        auto& instance = GetInstance();
        if (handle.index >= (int)instance.m_Instances.size()) return;
        InstanceSlot& slot = instance.m_Instances[handle.index];
        if (slot.generation != handle.generation) return;
        CallMethod(handle, "OnStart");
    }

    void ScriptEngine::LoadScript(InstanceHandle handle, const std::string& path)
    {
        auto& instance = GetInstance();
        if (handle.index >= instance.m_Instances.size()) return;
        InstanceSlot& slot = instance.m_Instances[handle.index];
        if (slot.generation != handle.generation) return;

        auto result = instance.s_LuaState.script_file(path, slot.env);
        if (!result.valid()) 
        {
            sol::error err = result;
            slot.has_error = true;
            AE_CORE_ERROR("[Lua Error] {0}", err.what());
            return;
        }
    }

    void ScriptEngine::DestroyInstance(InstanceHandle handle)
    {
        auto& instance = GetInstance();
        if (handle.index >= instance.m_Instances.size()) return;
        InstanceSlot& slot = instance.m_Instances[handle.index];
        if (slot.generation != handle.generation) return;
        CallMethod(handle, "OnDestroy");

        slot.env = sol::lua_nil;
        slot.generation++;
        instance.FreeList.push_back(handle.index);
    }

    void ScriptEngine::UpdateInstance(InstanceHandle handle, Timestep ts)
    {
        auto& instance = GetInstance();
        if (handle.index >= instance.m_Instances.size()) return;
        InstanceSlot& slot = instance.m_Instances[handle.index];
        if (slot.generation != handle.generation) return;
        if (slot.has_error) return;

        CallMethod(handle, "OnUpdate", (float)ts);
    }
}