#include "aepch.h"
#include "Aether/Scripting/ScriptTable.h"
#include "Aether/Core/Log.h"

namespace Aether {

    ScriptTable ScriptTable::FromSolObject(const sol::object& obj)
    {
        ScriptTable v;
        if (!obj.valid()) { v.m_Type = ScriptTable::Type::Nil; return v; }
        if (obj.is<bool>()) { v.m_Type = ScriptTable::Type::Bool; v.b = obj.as<bool>(); return v; }
        if (obj.is<float>()) { v.m_Type = ScriptTable::Type::Float; v.f = obj.as<float>(); return v; }
        if (obj.is<int>()) { v.m_Type = ScriptTable::Type::Int; v.i = obj.as<int>(); return v; }
        if (obj.is<std::string>()) { v.m_Type = ScriptTable::Type::String; v.m_Str = obj.as<std::string>(); return v; }
        if (obj.is<glm::vec3>())
        {
            glm::vec3 temp = obj.as<glm::vec3>();
            v.m_Type = ScriptTable::Type::Vec3;
            v.vec3.x = temp.x;
            v.vec3.y = temp.y;
            v.vec3.z = temp.z;
            return v;
        }
        if (obj.is<glm::quat>())
        {
            glm::quat temp = obj.as<glm::quat>();
            v.m_Type = ScriptTable::Type::Quat;
            v.quat.x = temp.x;
            v.quat.y = temp.y;
            v.quat.z = temp.z;
            v.quat.w = temp.w;
            return v;
        }
        if (obj.is<sol::table>())
        {
            v.m_Type = ScriptTable::Type::List;
            sol::table t = obj.as<sol::table>();
            for (size_t i = 1; i <= t.size(); i++)
                v.m_List.push_back(ScriptTable::FromSolObject(t[i]));
            return v;
        }
        AE_CORE_WARN("[ScriptTable] Unknown sol::object type, defaulting to Nil");
        return v;
    }

    sol::object ScriptTable::ToSolObject(sol::state& lua, const ScriptTable& val)
    {
        switch (val.m_Type)
        {
            case ScriptTable::Type::Nil: return sol::lua_nil;
            case ScriptTable::Type::Bool: return sol::make_object(lua, val.b);
            case ScriptTable::Type::Float: return sol::make_object(lua, val.f);
            case ScriptTable::Type::Int: return sol::make_object(lua, val.i);
            case ScriptTable::Type::String: return sol::make_object(lua, val.m_Str);
            case ScriptTable::Type::Vec3: return sol::make_object(lua, glm::vec3(val.vec3.x, val.vec3.y, val.vec3.z));
            case ScriptTable::Type::Quat: return sol::make_object(lua, glm::quat(val.quat.w, val.quat.x, val.quat.y, val.quat.z));
            case ScriptTable::Type::List:
            {
                sol::table t = lua.create_table();
                for (size_t i = 0; i < val.m_List.size(); i++)
                    t[i + 1] = ScriptTable::ToSolObject(lua, val.m_List[i]);
                return sol::make_object(lua, t);
            }
        }
        return sol::lua_nil;
    }

    ScriptTable ScriptTable::Make(bool val) { ScriptTable v; v.m_Type = Type::Bool; v.b = val; return v; }
    ScriptTable ScriptTable::Make(int val) { ScriptTable v; v.m_Type = Type::Int; v.i = val; return v; }
    ScriptTable ScriptTable::Make(float val) { ScriptTable v; v.m_Type = Type::Float; v.f = val; return v; }
    ScriptTable ScriptTable::Make(const std::string& val) { ScriptTable v; v.m_Type = Type::String; v.m_Str = val; return v; }
    ScriptTable ScriptTable::Make(const glm::vec3& val)
    {
        ScriptTable v; v.m_Type = Type::Vec3;
        v.vec3.x = val.x; v.vec3.y = val.y; v.vec3.z = val.z;
        return v;
    }
    ScriptTable ScriptTable::Make(const glm::quat& val)
    {
        ScriptTable v; v.m_Type = Type::Quat;
        v.quat.x = val.x; v.quat.y = val.y; v.quat.z = val.z; v.quat.w = val.w;
        return v;
    }
    ScriptTable ScriptTable::Make(const std::vector<ScriptTable>& val) { ScriptTable v; v.m_Type = Type::List; v.m_List = val; return v; }


    void ScriptTable::Set(bool val) { m_Type = Type::Bool; b = val; }
    void ScriptTable::Set(int val) { m_Type = Type::Int; i = val; }
    void ScriptTable::Set(float val) { m_Type = Type::Float; f = val; }
    void ScriptTable::Set(const std::string& val) { m_Type = Type::String; m_Str = val; }
    void ScriptTable::Set(const glm::vec3& val)
    {
        m_Type = Type::Vec3;
        vec3.x = val.x; vec3.y = val.y; vec3.z = val.z;
    }
    void ScriptTable::Set(const glm::quat& val)
    {
        m_Type = Type::Quat;
        quat.x = val.x; quat.y = val.y; quat.z = val.z; quat.w = val.w;
    }
    void ScriptTable::Set(const std::vector<ScriptTable>& val) { m_Type = Type::List; m_List = val; }
}