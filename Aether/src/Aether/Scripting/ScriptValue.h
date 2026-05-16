#pragma once
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
    };

    class ScriptArgs
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

    template<> bool ScriptValue::As() const;
    template<> int ScriptValue::As() const;
    template<> float ScriptValue::As() const;
    template<> std::string ScriptValue::As() const;
    template<> glm::vec3 ScriptValue::As() const;
    template<> ScriptList ScriptValue::As() const;
}