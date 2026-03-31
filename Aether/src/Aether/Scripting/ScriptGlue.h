#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Events/Event.h"
#include "Aether/Scene/Component.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/projection.hpp>
#include <glm/gtx/norm.hpp>

namespace Aether {

    struct Vec3Binding
    {
        using Type = glm::vec3;
        static constexpr const char* get_name() { return "Vec3"; }

        static constexpr auto get_props()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("x", 
                    AE_MAKE_LAMBDA((), (const Type& v), float, return v.x;), 
                    AE_MAKE_LAMBDA((), (Type& v, float val), void, v.x = val;)
                ),

                AE_REFLECT("y", 
                    AE_MAKE_LAMBDA((), (const Type& v), float, return v.y;), 
                    AE_MAKE_LAMBDA((), (Type& v, float val), void, v.y = val;)
                ),

                AE_REFLECT("z", 
                    AE_MAKE_LAMBDA((), (const Type& v), float, return v.z;), 
                    AE_MAKE_LAMBDA((), (Type& v, float val), void, v.z = val;)
                )
            );
        } 

        static constexpr auto get_ops()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("ADD", 
                    AE_MAKE_LAMBDA((), (const Type& a, const Type& b), auto, return a + b;)
                ),

                AE_REFLECT("SUB", 
                    AE_MAKE_LAMBDA((), (const Type& a, const Type& b), auto, return a - b;)
                ),

                AE_REFLECT("MUL", 
                    AE_MAKE_LAMBDA((), (const Type& a, const Type& b), auto, return a * b;),
                    AE_MAKE_LAMBDA((), (float a, const Type& b), auto, return a * b;),
                    AE_MAKE_LAMBDA((), (const Type& a, float b), auto, return a * b;)
                ),

                AE_REFLECT("DIV", 
                    AE_MAKE_LAMBDA((), (const Type& a, const Type& b), auto, return a / b;),
                    AE_MAKE_LAMBDA((), (const Type& a, float b), auto, return a / b;)
                )
            );
        }

        static constexpr auto get_methods()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("Length", 
                    AE_MAKE_LAMBDA((), (const Type& self), float, return glm::length(self);)
                ),

                AE_REFLECT("LengthSq", 
                    AE_MAKE_LAMBDA((), (const Type& self), float, return glm::length2(self);)
                ),
                
                AE_REFLECT("Normalize", 
                    AE_MAKE_LAMBDA((), (const Type& self), Type, return glm::normalize(self);)
                ),
                
                AE_REFLECT("IsZero", 
                    AE_MAKE_LAMBDA((), (const Type& self), bool, return self.x == 0 && self.y == 0 && self.z == 0;)
                )
            );
        }
    };

    struct QuatBinding
    {
        using Type = glm::quat;
        using VType = glm::vec3;
        static constexpr const char* get_name() { return "Quat"; }

        static constexpr auto get_props()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("x", 
                    AE_MAKE_LAMBDA((), (const Type& q), float, return q.x;), 
                    AE_MAKE_LAMBDA((), (Type& q, float val), void, q.x = val;)
                ),

                AE_REFLECT("y", 
                    AE_MAKE_LAMBDA((), (const Type& q), float, return q.y;), 
                    AE_MAKE_LAMBDA((), (Type& q, float val), void, q.y = val;)
                ),

                AE_REFLECT("z", 
                    AE_MAKE_LAMBDA((), (const Type& q), float, return q.z;), 
                    AE_MAKE_LAMBDA((), (Type& q, float val), void, q.z = val;)
                ),

                AE_REFLECT("w", 
                    AE_MAKE_LAMBDA((), (const Type& q), float, return q.w;), 
                    AE_MAKE_LAMBDA((), (Type& q, float val), void, q.w = val;)
                )
            );
        } 

        static constexpr auto get_ops()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("MUL", 
                    AE_MAKE_LAMBDA((), (const Type& a, const Type& b), auto, return a * b;),
                    AE_MAKE_LAMBDA((), (const Type& q, const VType& v), auto, return q * v;) 
                ),

                AE_REFLECT("DIV", 
                    AE_MAKE_LAMBDA((), (const Type& a, const Type& b), auto, return a * glm::inverse(b);)
                )
            );
        }

        static constexpr auto get_methods()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("Normalize", 
                    AE_MAKE_LAMBDA((), (const Type& self), Type, return glm::normalize(self);)
                ),
                
                AE_REFLECT("Inverse", 
                    AE_MAKE_LAMBDA((), (const Type& self), Type, return glm::inverse(self);)
                ),
                
                AE_REFLECT("ToEuler", 
                    AE_MAKE_LAMBDA((), (const Type& self), VType, return glm::eulerAngles(self);)
                ),
                
                AE_REFLECT("GetAxis", 
                    AE_MAKE_LAMBDA((), (const Type& self), VType, return glm::axis(self);)
                ),
                
                AE_REFLECT("GetAngle", 
                    AE_MAKE_LAMBDA((), (const Type& self), float, return glm::angle(self);)
                )
            );
        }
    }; 

    struct MathBinding
    {
        using VType = glm::vec3;
        using QType = glm::quat;

        static constexpr auto get_funcs()
        {
            return AE_REFLECT_LIST(
                
                AE_REFLECT("Vec3", 
                    AE_MAKE_LAMBDA((), (), VType, return VType(0.0f);),
                    AE_MAKE_LAMBDA((), (float val), VType, return VType(val);),
                    AE_MAKE_LAMBDA((), (float x, float y, float z), VType, return VType(x, y, z);)
                ),
                AE_REFLECT("Quat", 
                    AE_MAKE_LAMBDA((), (), QType, return QType(1.0f, 0.0f, 0.0f, 0.0f);),
                    AE_MAKE_LAMBDA((), (float w, float x, float y, float z), QType, return QType(w, x, y, z);),
                    AE_MAKE_LAMBDA((), (const VType& euler), QType, return QType(euler);) 
                ),

                AE_REFLECT("Vec3Zero", AE_MAKE_LAMBDA((), (), VType, return VType(0.0f);)),
                AE_REFLECT("Vec3Up", AE_MAKE_LAMBDA((), (), VType, return VType(0.0f, 1.0f, 0.0f);)),
                AE_REFLECT("Vec3Right", AE_MAKE_LAMBDA((), (), VType, return VType(1.0f, 0.0f, 0.0f);)),
                AE_REFLECT("Vec3Forward", AE_MAKE_LAMBDA((), (), VType, return VType(0.0f, 0.0f, -1.0f);)),
                AE_REFLECT("QuatIdentity", AE_MAKE_LAMBDA((), (), QType, return QType(1.0f, 0.0f, 0.0f, 0.0f);)),

                AE_REFLECT("Dot", 
                    AE_MAKE_LAMBDA((), (const VType& a, const VType& b), float, return glm::dot(a, b);),
                    AE_MAKE_LAMBDA((), (const QType& a, const QType& b), float, return glm::dot(a, b);)
                ),
                AE_REFLECT("Cross", 
                    AE_MAKE_LAMBDA((), (const VType& a, const VType& b), VType, return glm::cross(a, b);)
                ),
                AE_REFLECT("Lerp", 
                    AE_MAKE_LAMBDA((), (const VType& a, const VType& b, float t), VType, return glm::mix(a, b, t);),
                    AE_MAKE_LAMBDA((), (const QType& a, const QType& b, float t), QType, return glm::lerp(a, b, t);)
                ),
                AE_REFLECT("Slerp", 
                    AE_MAKE_LAMBDA((), (const QType& a, const QType& b, float t), QType, return glm::slerp(a, b, t);)
                ),
                AE_REFLECT("Distance", 
                    AE_MAKE_LAMBDA((), (const VType& a, const VType& b), float, return glm::distance(a, b);)
                ),
                AE_REFLECT("DistanceSq", 
                    AE_MAKE_LAMBDA((), (const VType& a, const VType& b), float, return glm::distance2(a, b);)
                ),
                AE_REFLECT("Reflect", 
                    AE_MAKE_LAMBDA((), (const VType& i, const VType& n), VType, return glm::reflect(i, n);)
                ),
                AE_REFLECT("Project", 
                    AE_MAKE_LAMBDA((), (const VType& v, const VType& normal), VType, return glm::proj(v, normal);)
                ),

                AE_REFLECT("FromAxisAngle",
                    AE_MAKE_LAMBDA((), (const VType& axis, float angle), QType, return glm::angleAxis(angle, axis);)
                )
            );
        }
    }; 

    struct TransformComponentBinding
    {
        using Type = TransformComponent;
        static constexpr const char* get_name() { return "TransformComponent"; }

        static constexpr auto get_props()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("Translation", 
                    AE_MAKE_LAMBDA((), (const Type& c), glm::vec3, return c.Translation;), 
                    AE_MAKE_LAMBDA((), (Type& c, const glm::vec3& val), void, c.Dirty = true; c.Translation = val;)
                ),

                AE_REFLECT("Rotation", 
                    AE_MAKE_LAMBDA((), (const Type& c), glm::quat, return c.Rotation;), 
                    AE_MAKE_LAMBDA((), (Type& c, const glm::quat& val), void, c.Dirty = true; c.Rotation = val;)
                ),

                AE_REFLECT("Scale", 
                    AE_MAKE_LAMBDA((), (const Type& c), glm::vec3, return c.Scale;), 
                    AE_MAKE_LAMBDA((), (Type& c, const glm::vec3& val), void, c.Dirty = true; c.Scale = val;)
                )
            );
        } 
    };
}