#include "Aether/Scripting/ScriptValue.h"
#include "Aether/Core/Log.h"

namespace Aether {

    template<> bool ScriptValue::As() const { return b; }
    template<> int ScriptValue::As() const { return i; }
    template<> float ScriptValue::As() const { return f; }
    template<> std::string ScriptValue::As() const { return str; }
    template<> glm::vec3 ScriptValue::As() const { return vec; }
    template<> ScriptList ScriptValue::As() const { return {list.data(), list.size()}; }
    
    ScriptValue FromSolObject(const sol::object& obj)
    {
        ScriptValue v;
        if (!obj.valid()) { v.type = ScriptValue::Type::Nil; return v; }
        if (obj.is<bool>()) { v.type = ScriptValue::Type::Bool; v.b = obj.as<bool>(); return v; }
        if (obj.is<int>()) { v.type = ScriptValue::Type::Int; v.i = obj.as<int>(); return v; }
        if (obj.is<float>()) { v.type = ScriptValue::Type::Float; v.f = obj.as<float>(); return v; }
        if (obj.is<std::string>()) { v.type = ScriptValue::Type::String; v.str = obj.as<std::string>(); return v; }
        if (obj.is<glm::vec3>()) { v.type = ScriptValue::Type::Vec3; v.vec = obj.as<glm::vec3>(); return v; }
        if (obj.is<sol::table>())
        {
            v.type = ScriptValue::Type::List;
            sol::table t = obj.as<sol::table>();
            for (size_t i = 1; i <= t.size(); i++)
                v.list.push_back(FromSolObject(t[i]));
            return v;
        }
        AE_CORE_WARN("[ScriptValue] Unknown sol::object type, defaulting to Nil");
        return v;
    }

    std::tuple<const ScriptValue*, uint32_t> ScriptArgs::GetArgs() const 
    { 
        return { args.data(), args.size()}; 
    }

    void ScriptArgs::Pushback(const ScriptValue& val) 
    { 
        args.push_back(val); 
    }
    
    uint32_t ScriptArgs::GetSize() const
    {
        return args.size();
    }

    sol::object ToSolObject(sol::state& lua, const ScriptValue& val)
    {
        switch (val.type)
        {
            case ScriptValue::Type::Nil: return sol::lua_nil;
            case ScriptValue::Type::Bool: return sol::make_object(lua, val.b);
            case ScriptValue::Type::Int: return sol::make_object(lua, val.i);
            case ScriptValue::Type::Float: return sol::make_object(lua, val.f);
            case ScriptValue::Type::String: return sol::make_object(lua, val.str);
            case ScriptValue::Type::Vec3: return sol::make_object(lua, val.vec);
            case ScriptValue::Type::List:
            {
                sol::table t = lua.create_table();
                for (size_t i = 0; i < val.list.size(); i++)
                    t[i + 1] = ToSolObject(lua, val.list[i]);
                return sol::make_object(lua, t);
            }
        }
        return sol::lua_nil;
    }
}