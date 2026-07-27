#pragma once

#include "Aether/Core/Base.h"
#include <vector>
#include <tuple>
#include <sol/sol.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Aether {

    class AETHER_API ScriptTable
    {
    public:
        enum class Type { Nil, Bool, Int, Float, String, Vec3, Quat, List };

        ScriptTable() = default;

        Type GetType() const { return m_Type; }
        bool IsList() const { return m_Type == Type::List; }

        static ScriptTable FromSolObject(const sol::object& obj);
        static sol::object ToSolObject(sol::state& lua, const ScriptTable& val);

        template<typename T> T As() const;

        static ScriptTable Make(bool val);
        static ScriptTable Make(int val);
        static ScriptTable Make(float val);
        static ScriptTable Make(const std::string& val);
        static ScriptTable Make(const glm::vec3& val);
        static ScriptTable Make(const glm::quat& val);
        static ScriptTable Make(const std::vector<ScriptTable>& val);

        void Set(bool val);
        void Set(int val);
        void Set(float val);
        void Set(const std::string& val);
        void Set(const glm::vec3& val);
        void Set(const glm::quat& val);
        void Set(const std::vector<ScriptTable>& val);

        void Pushback(const ScriptTable& val)
        {
            if (m_Type != Type::List) { m_Type = Type::List; m_List.clear(); }
            m_List.push_back(val);
        }

        uint32_t GetSize() const { return m_Type == Type::List ? (uint32_t)m_List.size() : 0; }

        template<typename T>
        T GetElement(uint32_t index) const
        {
            if (m_Type == Type::List && index < m_List.size()) return m_List[index].As<T>();
            return T{};
        }

        template<typename T>
        T Get() const
        {
            if (m_Type != Type::List) return As<T>();
            if (m_List.size() == 1) return m_List[0].As<T>();
            return T{};
        }

    private:
        Type m_Type = Type::Nil;
        union
        {
            bool b;
            int i;
            float f;
            struct { float x, y, z; } vec3;
            struct { float x, y, z, w; } quat;
        };
        std::string m_Str;
        std::vector<ScriptTable> m_List;
    };

    // As
    template<>
    inline bool ScriptTable::As<bool>() const { return b; }

    template<>
    inline int ScriptTable::As<int>() const { return i; }

    template<>
    inline float ScriptTable::As<float>() const { return f; }

    template<>
    inline std::string ScriptTable::As<std::string>() const { return m_Str; }

    template<>
    inline glm::vec3 ScriptTable::As<glm::vec3>() const { return glm::vec3(vec3.x, vec3.y, vec3.z); }

    template<>
    inline glm::quat ScriptTable::As<glm::quat>() const { return glm::quat(quat.w, quat.x, quat.y, quat.z); }
}