#pragma once

#include "Aether/Core/Base.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/gtx/norm.hpp>

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
        AE_ATTB_LIST(
            AE_REFLECT_ATTB(x),
            AE_REFLECT_ATTB(y),
            AE_REFLECT_ATTB(z)
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

        AE_ATTB_LIST(
            AE_REFLECT_ATTB(x),
            AE_REFLECT_ATTB(y),
            AE_REFLECT_ATTB(z),
            AE_REFLECT_ATTB(w)
        )
    };

    inline float Dot(const Vec3& a, const Vec3& b) { return glm::dot((glm::vec3)a, (glm::vec3)b); }
    inline float Length(const Vec3& v) { return glm::length((glm::vec3)v); }
    inline float Length2(const Vec3& v) { return glm::length2((glm::vec3)v); }
    inline float Distance(const Vec3& a, const Vec3& b) { return glm::distance((glm::vec3)a, (glm::vec3)b); }
    inline float Distance2(const Vec3& a, const Vec3& b) { glm::vec3 diff = (glm::vec3)a - (glm::vec3)b; return glm::dot(diff, diff); }
    inline float Angle(const Vec3& a, const Vec3& b) { return glm::acos(glm::clamp(glm::dot(glm::normalize((glm::vec3)a), glm::normalize((glm::vec3)b)), -1.0f, 1.0f)); }

    inline Vec3 Cross(const Vec3& a, const Vec3& b) { return glm::cross((glm::vec3)a, (glm::vec3)b); }
    inline Vec3 Normalize(const Vec3& v) { return glm::normalize((glm::vec3)v); }
    inline Vec3 Lerp(const Vec3& start, const Vec3& end, float t) { return glm::mix((glm::vec3)start, (glm::vec3)end, t); }
    inline Vec3 Reflect(const Vec3& v, const Vec3& norm) { return glm::reflect((glm::vec3)v, (glm::vec3)norm); }
    inline Vec3 Project(const Vec3& v, const Vec3& onto) { return glm::proj((glm::vec3)v, (glm::vec3)onto); }

    inline float Length(const Quat& q) { return glm::length((glm::quat)q); }
    inline float Dot(const Quat& a, const Quat& b) { return glm::dot((glm::quat)a, (glm::quat)b); }

    inline Quat Normalize(const Quat& q) { return glm::normalize((glm::quat)q); }
    inline Quat Conjugate(const Quat& q) { return glm::conjugate((glm::quat)q); }
    inline Quat Inverse(const Quat& q) { return glm::inverse((glm::quat)q); }
    inline Quat Slerp(const Quat& q1, const Quat& q2, float t) { return glm::slerp((glm::quat)q1, (glm::quat)q2, t); }
    inline Quat FromAxisAngle(const Vec3& axis, float angle) { return glm::angleAxis(angle, (glm::vec3)axis); }
    inline Quat FromEuler(const Vec3& v) { return glm::quat(glm::vec3(v.x, v.y, v.z)); }

    inline Vec3 Rotate(const Quat& q, const Vec3& v) { return (glm::quat)q * (glm::vec3)v; }
    inline Vec3 ToEuler(const Quat& q) { return glm::eulerAngles((glm::quat)q); }

    inline void ToAxisAngle(const Quat& q, Vec3& axis, float& angle) 
    { 
        angle = glm::angle((glm::quat)q); 
        axis = glm::axis((glm::quat)q); 
    }
}