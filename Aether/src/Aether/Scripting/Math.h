#pragma once

#include "Aether/Core/Base.h"
#include <glm/glm.hpp>
#include <glm/gtx/projection.hpp>

namespace Aether::Math
{
    struct Vec3
    {
        using Self = Vec3;
        float x, y, z;

        Vec3(const glm::vec3& origin)
            : x(origin.x), y(origin.y), z(origin.z) {}

        operator glm::vec3() const 
        {
            return glm::vec3(x, y, z); 
        }

        AE_REFLECT_NAME("Vec3")

        AE_OP(ADD, V, V, Vec3, Vec3, Vec3, glm::vec3, glm::vec3, +)
        AE_OP(SUB, V, V, Vec3, Vec3, Vec3, glm::vec3, glm::vec3, -)
        AE_OP(MUL, V, V, Vec3, Vec3, Vec3, glm::vec3, glm::vec3, *)
        AE_OP(DIV, V, V, Vec3, Vec3, Vec3, glm::vec3, glm::vec3, /)

        AE_OP_COM(MUL, V, F, Vec3, float, Vec3, glm::vec3, float, *)
        AE_OP_COM(DIV, V, F, Vec3, float, Vec3, glm::vec3, float, /)

        AE_OP_LIST(
            AE_REFLECT_OP(ADD, V, V),
            AE_REFLECT_OP(SUB, V, V),
            AE_REFLECT_OP(MUL, V, V),
            AE_REFLECT_OP_COM(MUL, V, F),
            AE_REFLECT_OP(DIV, V, V),
            AE_REFLECT_OP_COM(DIV, V, F)
        )
        AE_PROP_LIST(
            AE_REFLECT_PROP(x),
            AE_REFLECT_PROP(y),
            AE_REFLECT_PROP(z)
        )
        
    };

    struct Quat
    {
        using Self = Quat;
        float x, y, z, w;

        Quat(const glm::quat& origin)
            : x(origin.x), y(origin.y), z(origin.z), w(origin.w) {}

        operator glm::quat() const 
        {
            return glm::quat(x, y, z, w);
        }

        AE_REFLECT_NAME("Quat")

        AE_OP(MUL, Q, Q, Quat, Quat, Quat, glm::quat, glm::quat, *)
        AE_OP(MUL, Q, V, Quat, Vec3, Vec3, glm::quat, glm::vec3, *)
        AE_OP(DIV, Q, F, Quat, float, Quat, glm::quat, float, /)
        AE_OP_COM(MUL, Q, F, Quat, float, Quat, glm::quat, float, *)

        AE_OP_LIST(
            AE_REFLECT_OP(MUL, Q, Q),
            AE_REFLECT_OP(MUL, Q, V),
            AE_REFLECT_OP_COM(MUL, Q, F),
            AE_REFLECT_OP(DIV, Q, F)
        )

        AE_PROP_LIST(
            AE_REFLECT_PROP(x),
            AE_REFLECT_PROP(y),
            AE_REFLECT_PROP(z),
            AE_REFLECT_PROP(w)
        )
    };

    float Dot(const Vec3& a, const Vec3& b) { return glm::dot((glm::vec3)a, (glm::vec3)b); }
    float Length(const Vec3& v) { return glm::length((glm::vec3)v); }
    float Length2(const Vec3& v) { return glm::length2((glm::vec3)v); }
    float Distance(const Vec3& a, const Vec3& b) { return glm::distance((glm::vec3)a, (glm::vec3)b); }
    float Distance2(const Vec3& a, const Vec3& b) { glm::vec3 diff = (glm::vec3)a - (glm::vec3)b; return glm::dot(diff, diff); }
    float Angle(const Vec3& a, const Vec3& b) { return glm::acos(glm::clamp(glm::dot(glm::normalize((glm::vec3)a), glm::normalize((glm::vec3)b)), -1.0f, 1.0f)); }

    Vec3 Cross(const Vec3& a, const Vec3& b) { return glm::cross((glm::vec3)a, (glm::vec3)b); }
    Vec3 Normalize(const Vec3& v) { return glm::normalize((glm::vec3)v); }
    Vec3 Lerp(const Vec3& start, const Vec3& end, float t) { return glm::mix((glm::vec3)start, (glm::vec3)end, t); }
    Vec3 Reflect(const Vec3& v, const Vec3& norm) { return glm::reflect((glm::vec3)v, (glm::vec3)norm); }
    Vec3 Project(const Vec3& v, const Vec3& onto) { return glm::proj((glm::vec3)v, (glm::vec3)onto); }

    float Length(const Quat& q) { return glm::length((glm::quat)q); }
    float Dot(const Quat& a, const Quat& b) { return glm::dot((glm::quat)a, (glm::quat)b); }

    Quat Normalize(const Quat& q) { return glm::normalize((glm::quat)q); }
    Quat Conjugate(const Quat& q) { return glm::conjugate((glm::quat)q); }
    Quat Inverse(const Quat& q) { return glm::inverse((glm::quat)q); }
    Quat Slerp(const Quat& q1, const Quat& q2, float t) { return glm::slerp((glm::quat)q1, (glm::quat)q2, t); }
    Quat FromAxisAngle(const Vec3& axis, float angle) { return glm::angleAxis(angle, (glm::vec3)axis); }
    Quat FromEuler(const Vec3& v) { return glm::quat(glm::vec3(v.x, v.y, v.z)); }

    Vec3 Rotate(const Quat& q, const Vec3& v) { return (glm::quat)q * (glm::vec3)v; }
    Vec3 ToEuler(const Quat& q) { return glm::eulerAngles((glm::quat)q); }

    void ToAxisAngle(const Quat& q, Vec3& axis, float& angle) 
    { 
        angle = glm::angle((glm::quat)q); 
        axis = glm::axis((glm::quat)q); 
    }
}