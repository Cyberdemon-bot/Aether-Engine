#pragma once

#include "Aether/Core/Base.h"
#include <vector>
#include <tuple>
#include <sol/sol.hpp>
#include <glm/glm.hpp>

namespace Aether {
    struct ScriptValue
    {
        enum class Type { Nil, Bool, Int, Float, String, Vec3, List };
        Type type = Type::Nil;
        union { bool b; int i; float f; };
        std::string str;
        glm::vec3 vec;
        std::vector<ScriptValue> list;

        template<typename T> T As() const;
        template<typename T> static ScriptValue Make(const T& val);
        template<typename T> void Set(const T& val);
    };

    class AETHER_API ScriptArgs
    {
    public:
        std::tuple<const ScriptValue*, uint32_t> GetArgs() const;
        void Pushback(const ScriptValue& val);

        template<typename T>
        T GetElement(uint32_t index) const
        {
            if (index < GetSize()) return args[index].As<T>();
            return T{};
        }
        
        uint32_t GetSize() const;
    private:
        std::vector<ScriptValue> args;
    };

    ScriptValue FromSolObject(const sol::object& obj);
    sol::object ToSolObject(sol::state& lua, const ScriptValue& val);

    typedef std::tuple<const ScriptValue*, size_t> ScriptList;

    // As
    template<>
    inline bool ScriptValue::As<bool>() const { return b; }

    template<>
    inline int ScriptValue::As<int>() const { return i; }

    template<>
    inline float ScriptValue::As<float>() const { return f; }

    template<>
    inline std::string ScriptValue::As<std::string>() const { return str; }

    template<>
    inline glm::vec3 ScriptValue::As<glm::vec3>() const { return vec; }

    template<>
    inline ScriptList ScriptValue::As<ScriptList>() const { return {list.data(), list.size()}; }

    //Make
    template<>
    inline ScriptValue ScriptValue::Make<bool>(const bool& val) 
    { 
        ScriptValue v; v.type = Type::Bool; v.b = val; return v; 
    }

    template<>
    inline ScriptValue ScriptValue::Make<int>(const int& val) 
    { 
        ScriptValue v; v.type = Type::Int; v.i = val; return v; 
    }

    template<>
    inline ScriptValue ScriptValue::Make<float>(const float& val) 
    { 
        ScriptValue v; v.type = Type::Float; v.f = val; return v; 
    }

    template<>
    inline ScriptValue ScriptValue::Make<std::string>(const std::string& val) 
    { 
        ScriptValue v; v.type = Type::String; v.str = val; return v; 
    }

    template<>
    inline ScriptValue ScriptValue::Make<glm::vec3>(const glm::vec3& val) 
    { 
        ScriptValue v; v.type = Type::Vec3; v.vec = val; return v; 
    }

    template<>
    inline ScriptValue ScriptValue::Make<std::vector<ScriptValue>>(const std::vector<ScriptValue>& val) 
    { 
        ScriptValue v; v.type = Type::List; v.list = val; return v; 
    }

    //Set
    template<>
    inline void ScriptValue::Set<bool>(const bool& val) 
    { 
        type = Type::Bool; b = val; 
    }

    template<>
    inline void ScriptValue::Set<int>(const int& val) 
    { 
        type = Type::Int; i = val; 
    }

    template<>
    inline void ScriptValue::Set<float>(const float& val) 
    { 
        type = Type::Float; f = val; 
    }

    template<>
    inline void ScriptValue::Set<std::string>(const std::string& val) 
    { 
        type = Type::String; str = val; 
    }

    template<>
    inline void ScriptValue::Set<glm::vec3>(const glm::vec3& val) 
    { 
        type = Type::Vec3; vec = val; 
    }

    template<>
    inline void ScriptValue::Set<std::vector<ScriptValue>>(const std::vector<ScriptValue>& val) 
    { 
        type = Type::List; list = val; 
    }
}