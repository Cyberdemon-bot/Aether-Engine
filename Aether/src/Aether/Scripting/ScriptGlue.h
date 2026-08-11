#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Events/Event.h"
#include "Aether/Scene/Component.h"
#include "Aether/Core/Input.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Scene/SceneCamera.h"
#include "Aether/Scripting/ScriptEngine.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/projection.hpp>
#include <glm/gtx/norm.hpp>

namespace Aether {

    struct U64
    {
        uint64_t value = 0;
        U64() = default;
        U64(uint64_t v) : value(v) {}
    };

    struct U64Binding
    {
        using Type = U64;
        static constexpr const char* get_name() { return "U64"; }

        static constexpr auto get_props()
        {
            return AE_REFLECT_LIST();
        }

        static constexpr auto get_ops()
        {
            return AE_REFLECT_LIST();
        }

        static constexpr auto get_methods()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("ToString", 
                    AE_MAKE_LAMBDA((), (const Type& self), std::string, return std::to_string(self.value);)
                )
            );
        }
    };

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

    struct CollisionBinding
    {
        using Type = CollisionData;
        using VType = glm::vec3;
        static constexpr const char* get_name() { return "CollisionData"; }

        static constexpr auto get_props()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("EntityID", 
                    AE_MAKE_LAMBDA((), (const Type& self), U64,
                        return U64(self.entityID);
                    )
                ),

                AE_REFLECT("Type", 
                    AE_MAKE_LAMBDA((), (const Type& self), int,
                        return static_cast<int>(self.type);
                    )
                ),

                AE_REFLECT("ContactPoint", 
                    AE_MAKE_LAMBDA((), (const Type& self), VType,
                        return self.contactPoint;
                    )
                ),

                AE_REFLECT("ContactNormal", 
                    AE_MAKE_LAMBDA((), (const Type& self), VType,
                        return self.contactNormal;
                    )
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

    struct TransformBinding
    {
        using Type = TransformComponent;
        static constexpr const char* get_name() { return "Transform"; }

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

    struct LightParamBinding
    {
        using Type = LightParam;
        static constexpr const char* get_name() { return "LightParam"; }

        static constexpr auto get_props()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("type",
                    AE_MAKE_LAMBDA((), (const Type& c), int, return static_cast<int>(c.type);),
                    AE_MAKE_LAMBDA((), (Type& c, int val), void, c.type = static_cast<LightType>(val);)
                ),

                AE_REFLECT("position",
                    AE_MAKE_LAMBDA((), (const Type& c), glm::vec3, return c.position;),
                    AE_MAKE_LAMBDA((), (Type& c, const glm::vec3& val), void, c.position = val;)
                ),

                AE_REFLECT("direction",
                    AE_MAKE_LAMBDA((), (const Type& c), glm::vec3, return c.direction;),
                    AE_MAKE_LAMBDA((), (Type& c, const glm::vec3& val), void, c.direction = val;)
                ),

                AE_REFLECT("color",
                    AE_MAKE_LAMBDA((), (const Type& c), glm::vec3, return c.color;),
                    AE_MAKE_LAMBDA((), (Type& c, const glm::vec3& val), void, c.color = val;)
                ),

                AE_REFLECT("intensity",
                    AE_MAKE_LAMBDA((), (const Type& c), float, return c.intensity;),
                    AE_MAKE_LAMBDA((), (Type& c, float val), void, c.intensity = val;)
                ),

                AE_REFLECT("range",
                    AE_MAKE_LAMBDA((), (const Type& c), float, return c.range;),
                    AE_MAKE_LAMBDA((), (Type& c, float val), void, c.range = val;)
                ),

                AE_REFLECT("innerCone",
                    AE_MAKE_LAMBDA((), (const Type& c), float, return c.innerCone;),
                    AE_MAKE_LAMBDA((), (Type& c, float val), void, c.innerCone = val;)
                ),

                AE_REFLECT("outerCone",
                    AE_MAKE_LAMBDA((), (const Type& c), float, return c.outerCone;),
                    AE_MAKE_LAMBDA((), (Type& c, float val), void, c.outerCone = val;)
                ),

                AE_REFLECT("castShadows",
                    AE_MAKE_LAMBDA((), (const Type& c), bool, return c.castShadows;),
                    AE_MAKE_LAMBDA((), (Type& c, bool val), void, c.castShadows = val;)
                )
            );
        }
    };

    struct SceneCameraBinding
    {
        using Type = SceneCamera;
        static constexpr const char* get_name() { return "SceneCamera"; }

        static constexpr auto get_props()
        {
            return AE_REFLECT_LIST(

                AE_REFLECT("projectionType",
                    AE_MAKE_LAMBDA((), (const Type& c), int,
                        return static_cast<int>(c.GetProjectionType());
                    ),
                    AE_MAKE_LAMBDA((), (Type& c, int val), void,
                        c.SetProjectionType(static_cast<SceneCamera::ProjectionType>(val));
                    )
                ),
                AE_REFLECT("fov",
                    AE_MAKE_LAMBDA((), (const Type& c), float, return c.GetPerspectiveVerticalFOV();),
                    AE_MAKE_LAMBDA((), (Type& c, float val), void, c.SetPerspectiveVerticalFOV(val);)
                ),
                AE_REFLECT("perspNear",
                    AE_MAKE_LAMBDA((), (const Type& c), float, return c.GetPerspectiveNearClip();),
                    AE_MAKE_LAMBDA((), (Type& c, float val), void, c.SetPerspectiveNearClip(val);)
                ),
                AE_REFLECT("perspFar",
                    AE_MAKE_LAMBDA((), (const Type& c), float, return c.GetPerspectiveFarClip();),
                    AE_MAKE_LAMBDA((), (Type& c, float val), void, c.SetPerspectiveFarClip(val);)
                ),
                AE_REFLECT("orthoSize",
                    AE_MAKE_LAMBDA((), (const Type& c), float, return c.GetOrthographicSize();),
                    AE_MAKE_LAMBDA((), (Type& c, float val), void, c.SetOrthographicSize(val);)
                ),
                AE_REFLECT("orthoNear",
                    AE_MAKE_LAMBDA((), (const Type& c), float, return c.GetOrthographicNearClip();),
                    AE_MAKE_LAMBDA((), (Type& c, float val), void, c.SetOrthographicNearClip(val);)
                ),
                AE_REFLECT("orthoFar",
                    AE_MAKE_LAMBDA((), (const Type& c), float, return c.GetOrthographicFarClip();),
                    AE_MAKE_LAMBDA((), (Type& c, float val), void, c.SetOrthographicFarClip(val);)
                ),

                AE_REFLECT("aspectRatio",
                    AE_MAKE_LAMBDA((), (const Type& c), float, return c.GetAspectRatio();)
                )
            );
        }

        static constexpr auto get_methods()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("SetPerspective",
                    AE_MAKE_LAMBDA((), (Type& c, float fov, float nearClip, float farClip), void,
                        c.SetPerspective(fov, nearClip, farClip);
                    )
                ),
                AE_REFLECT("SetOrthographic",
                    AE_MAKE_LAMBDA((), (Type& c, float size, float nearClip, float farClip), void,
                        c.SetOrthographic(size, nearClip, farClip);
                    )
                ),
                AE_REFLECT("SetViewportSize",
                    AE_MAKE_LAMBDA((), (Type& c, int width, int height), void,
                        c.SetViewportSize((uint32_t)width, (uint32_t)height);
                    )
                ),
                AE_REFLECT("SetView",
                    AE_MAKE_LAMBDA((), (Type& c, const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up), void,
                        c.SetView(glm::inverse(glm::lookAt(pos, target, up)));
                    )
                )
            );
        }
    };

    struct SelfBinding
    {
        using Type = SelfContext;
        static constexpr const char* get_name() { return "SelfContext"; }

        static constexpr auto get_props()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("Transform",
                    AE_MAKE_LAMBDA((), (const Type& ctx), TransformComponent*,
                        return &ctx.context.scene->GetComponent<TransformComponent>(ctx.context.entity);
                    ),
                    AE_MAKE_LAMBDA((), (Type& ctx, const TransformComponent& val), void,
                        ctx.context.scene->GetComponent<TransformComponent>(ctx.context.entity) = val;
                    )
                ),
                AE_REFLECT("LightConfig",
                    AE_MAKE_LAMBDA((), (const Type& ctx), LightParam*,
                        if (!ctx.context.scene->HasComponent<LightComponent>(ctx.context.entity)) return nullptr;
                        return &ctx.context.scene->GetComponent<LightComponent>(ctx.context.entity).Config;
                    ),
                    AE_MAKE_LAMBDA((), (Type& ctx, const LightParam& val), void,
                        if (!ctx.context.scene->HasComponent<LightComponent>(ctx.context.entity)) return;
                        ctx.context.scene->GetComponent<LightComponent>(ctx.context.entity).Config = val;
                    )
                ),
                AE_REFLECT("Camera",
                    AE_MAKE_LAMBDA((), (const Type& ctx), SceneCamera*,
                        if (!ctx.context.scene->HasComponent<CameraComponent>(ctx.context.entity)) return nullptr;
                        return &ctx.context.scene->GetComponent<CameraComponent>(ctx.context.entity).Camera;
                    ),
                    AE_MAKE_LAMBDA((), (Type& ctx, const SceneCamera& val), void,
                        if (!ctx.context.scene->HasComponent<CameraComponent>(ctx.context.entity)) return;
                        ctx.context.scene->GetComponent<CameraComponent>(ctx.context.entity).Camera = val;
                    )
                ),

                AE_REFLECT("IsPrimaryCamera",
                    AE_MAKE_LAMBDA((), (const Type& ctx), bool,
                        if (!ctx.context.scene->HasComponent<CameraComponent>(ctx.context.entity)) return false;
                        return ctx.context.scene->GetComponent<CameraComponent>(ctx.context.entity).Primary;
                    ),
                    AE_MAKE_LAMBDA((), (Type& ctx, bool val), void,
                        if (!ctx.context.scene->HasComponent<CameraComponent>(ctx.context.entity)) return;
                        ctx.context.scene->GetComponent<CameraComponent>(ctx.context.entity).Primary = val;
                    )
                ),
                AE_REFLECT("EntityID",
                    AE_MAKE_LAMBDA((), (const Type& ctx), uint64_t,
                        return uint64_t(ctx.context.scene->GetComponent<IDComponent>(ctx.context.entity).ID);
                    )
                ), 
                AE_REFLECT("ExecOrder",
                    AE_MAKE_LAMBDA((), (const Type& ctx), int,
                        return ctx.context.slot->exec_order;
                    ),
                    AE_MAKE_LAMBDA((), (Type& ctx, int val), void,
                        ctx.context.engine->IsExecChanged = true; 
                        ctx.context.slot->exec_order = val;
                    )
                )
            );
        }

        static constexpr auto get_methods()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("Expose",
                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& name, sol::protected_function func), void,
                        ctx.context.slot->exposed_funcs.push_back({name, func});
                    )
                ),
                AE_REFLECT("DestroyMyself",
                    AE_MAKE_LAMBDA((), (Type& ctx), void,
                        Entity e = static_cast<Entity>(ctx.context.entity);
                        ctx.context.scene->DestroyEntity(e);
                    )
                ),
                AE_REFLECT("GetWorldPosition",
                    AE_MAKE_LAMBDA((), (Type& ctx), glm::vec3,
                        Entity e = static_cast<Entity>(ctx.context.entity);
                        return ctx.context.scene->GetWorldPosition(e);
                    )
                ),
                AE_REFLECT("BreakParent",
                    AE_MAKE_LAMBDA((), (Type& ctx), void,
                        Entity e = static_cast<Entity>(ctx.context.entity);
                        ctx.context.scene->BreakParent(e);
                    )
                ),
                AE_REFLECT("MakeParent",
                    AE_MAKE_LAMBDA((), (Type& ctx, uint64_t parentId), void,
                        Entity e = static_cast<Entity>(ctx.context.entity);
                        Entity parent = ctx.context.scene->FindEntity((UUID(parentId)));
                        ctx.context.scene->MakeParent(e, parent);
                    )
                )
            );
        }
    };

    struct SceneBinding
    {
        using Type = SceneContext;
        static constexpr const char* get_name() { return "SceneContext"; }

        static constexpr auto get_methods()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("FindByTag",
                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& name), std::vector<U64>,
                        auto results = ctx.context.scene->FindEntity(name);
                        std::vector<U64> ids;
                        ids.reserve(results.size());
                        for (auto& e : results) ids.push_back(U64(ctx.context.scene->GetComponent<IDComponent>(e).ID));
                        return ids;
                    )
                ),

                AE_REFLECT("IsValid",
                    AE_MAKE_LAMBDA((), (Type& ctx, U64 id), bool,
                        Entity e = ctx.context.scene->FindEntity(UUID(id.value));
                        return ctx.context.scene->IsValid(e);
                    )
                ),
                AE_REFLECT("SafeCall",
                    AE_MAKE_LAMBDA((), (Type& ctx, U64 targetId, const std::string& name, sol::variadic_args args), sol::object,
                        Entity target = ctx.context.scene->FindEntity((UUID(targetId.value)));
                        if (!ctx.context.scene->IsValid(target)) return sol::lua_nil;
                        if (!ctx.context.scene->HasComponent<ScriptComponent>(target)) return sol::lua_nil;
                        auto& sc = ctx.context.scene->GetComponent<ScriptComponent>(target);
                        return ctx.context.engine->CallSafeInstanceAPI(sc.ScriptHandle, name, sol::as_args(args));
                    )
                ),
                AE_REFLECT("DirectCall",
                    AE_MAKE_LAMBDA((), (Type& ctx, U64 targetId, const std::string& name, sol::variadic_args args), sol::object,
                        Entity target = ctx.context.scene->FindEntity((UUID(targetId.value)));
                        if (!ctx.context.scene->IsValid(target)) return sol::lua_nil;
                        if (!ctx.context.scene->HasComponent<ScriptComponent>(target)) return sol::lua_nil;
                        auto& sc = ctx.context.scene->GetComponent<ScriptComponent>(target);
                        return ctx.context.engine->CallDirectInstanceAPI(sc.ScriptHandle, name, sol::as_args(args));
                    )
                )
            );
        }
    };

    struct InputBinding
    {
        using Vec2Tuple = std::tuple<float, float>;
        static constexpr auto get_funcs()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("IsKeyPressed",
                    AE_MAKE_LAMBDA((), (int keyCode), bool,
                        return Input::IsKeyPressed(static_cast<Key::KeyCode>(keyCode));)
                ),
                AE_REFLECT("IsMouseButtonPressed",
                    AE_MAKE_LAMBDA((), (int mouseCode), bool,
                        return Input::IsMouseButtonPressed(static_cast<Mouse::MouseCode>(mouseCode));)
                ),
                AE_REFLECT("GetMousePosition",
                    AE_MAKE_LAMBDA((), (), Vec2Tuple,
                        auto pos = Input::GetMousePosition();
                        return Vec2Tuple{pos.x, pos.y};)
                ),
                AE_REFLECT("GetMouseX",
                    AE_MAKE_LAMBDA((), (), float, return Input::GetMouseX();)
                ),
                AE_REFLECT("GetMouseY",
                    AE_MAKE_LAMBDA((), (), float, return Input::GetMouseY();)
                )
            );
        }
    };

    struct RaycastResultBinding
    {
        using Type = RaycastResult;
        using VType = glm::vec3;
        static constexpr const char* get_name() { return "RaycastResult"; }

        static constexpr auto get_props()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("Hit",
                    AE_MAKE_LAMBDA((), (const Type& self), bool, return self.Hit;)
                ),
                AE_REFLECT("Position",
                    AE_MAKE_LAMBDA((), (const Type& self), VType, return self.Position;)
                ),
                AE_REFLECT("Normal",
                    AE_MAKE_LAMBDA((), (const Type& self), VType, return self.Normal;)
                ),
                AE_REFLECT("Distance",
                    AE_MAKE_LAMBDA((), (const Type& self), float, return self.Distance;)
                ),
                AE_REFLECT("HitBody",
                    AE_MAKE_LAMBDA((), (const Type& self), U64,
                        return U64(self.HitBody.Blend());
                    )
                )
            );
        }
    };

    struct PhysicsBinding
    {
        using Type = PhysicsContext;
        using VType = glm::vec3;
        static constexpr const char* get_name() { return "PhysicsContext"; }
        static constexpr auto get_methods() 
        {
            return AE_REFLECT_LIST(

                AE_REFLECT("AddForce",
                    AE_MAKE_LAMBDA((), (Type& ctx, const VType& force), void,
                        if (!ctx.context.scene->HasComponent<ColliderComponent>(ctx.context.entity)) return;
                        auto handle = ctx.context.scene->GetComponent<ColliderComponent>(ctx.context.entity).ColliderHandle;
                        ctx.context.physys->AddForce(ctx.context.scene->GetPhysicsInstance(), handle, force);
                    )
                ),

                AE_REFLECT("SetVelocity",
                    AE_MAKE_LAMBDA((), (Type& ctx, const VType& velocity), void,
                        if (!ctx.context.scene->HasComponent<ColliderComponent>(ctx.context.entity)) return;
                        auto handle = ctx.context.scene->GetComponent<ColliderComponent>(ctx.context.entity).ColliderHandle;
                        ctx.context.physys->SetVelocity(ctx.context.scene->GetPhysicsInstance(), handle, velocity);
                    )
                ),

                AE_REFLECT("SetGravity",
                    AE_MAKE_LAMBDA((), (Type& ctx, const VType& gravity), void,
                        ctx.context.physys->SetGravity(ctx.context.scene->GetPhysicsInstance(), gravity);
                    )
                ),

                AE_REFLECT("CanMove",
                    AE_MAKE_LAMBDA((), (Type& ctx, const PhysTransform& target), bool,
                        if (!ctx.context.scene->HasComponent<ColliderComponent>(ctx.context.entity)) return false;
                        auto handle = ctx.context.scene->GetComponent<ColliderComponent>(ctx.context.entity).ColliderHandle;
                        return ctx.context.physys->CanMove(ctx.context.scene->GetPhysicsInstance(), handle, target);
                    )
                ),

                AE_REFLECT("CastRay",
                    AE_MAKE_LAMBDA((), (Type& ctx, const VType& origin, const VType& direction, float distance), RaycastResult,
                        return ctx.context.physys->CastRay(ctx.context.scene->GetPhysicsInstance(), origin, direction, distance);
                    )
                ),

                AE_REFLECT("CastRayAll",
                    AE_MAKE_LAMBDA((), (Type& ctx, const VType& origin, const VType& direction, float distance), std::vector<RaycastResult>,
                        return ctx.context.physys->CastRayAll(ctx.context.scene->GetPhysicsInstance(), origin, direction, distance);
                    )
                ),

                AE_REFLECT("GetEntity",
                    AE_MAKE_LAMBDA((), (Type& ctx, U64 body), U64,
                        uint64_t ud = ctx.context.physys->GetUserData(ctx.context.scene->GetPhysicsInstance(), Handle<RigidBody>::FromBlend(body.value));
                        return U64(ud);
                    )
                )
            );
        }
    };

    struct CoroutineBinding
    {
        using Type = CoroutineContext;
        static constexpr const char* get_name() { return "CoroutineContext"; }

        static inline sol::table MakeAwaitable(sol::state_view lua, uint64_t blend, uint32_t type, float timeout)
        {
            sol::table t = lua.create_table(3, 0);
            t[1] = U64(blend);
            t[2] = type;
            t[3] = timeout;
            return t;
        }

        static constexpr auto get_methods()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("async",
                    AE_MAKE_LAMBDA((), (Type& ctx, sol::function func), U64,
                        auto handle = ctx.context.coroutine_manager->StartCoroutine(func, ctx.context.handle);
                        return U64(handle.Blend());
                    )
                ),

                AE_REFLECT("stop",
                    AE_MAKE_LAMBDA((), (Type& ctx, U64 handle), void,
                        ctx.context.coroutine_manager->StopCoroutine(Handle<CoroutineTask>::FromBlend(handle.value));
                    )
                ),

                AE_REFLECT("await",
                    AE_MAKE_LAMBDA((), (Type& ctx, sol::this_state ts), int,
                        lua_State* L = ts;
                        lua_pushthread(L);
                        sol::thread current = sol::stack::pop<sol::thread>(L);
                        auto task = ctx.context.coroutine_manager->GetCurrentRunningTask(current);
                        ctx.context.coroutine_manager->YieldTask(task);
                        return lua_yield(L, 0);
                    ),
                    AE_MAKE_LAMBDA((), (Type& ctx, sol::table awaitable, sol::this_state ts), int,
                        uint64_t handle = awaitable.get<U64>(1).value;
                        uint32_t type = awaitable[2];
                        float timeout = awaitable[3];

                        lua_State* L = ts;
                        lua_pushthread(L);
                        sol::thread current = sol::stack::pop<sol::thread>(L);
                        auto task = ctx.context.coroutine_manager->GetCurrentRunningTask(current);

                        sol::state_view lua_view(L);
                        lua_State* main_L = lua_view.lua_state(); 
                        auto promise_handle = Handle<Promise>::FromBlend(handle);
                        if (promise_handle.IsValid() && ctx.context.promise_manager->GetPromise(promise_handle))
                        {
                            ctx.context.promise_manager->Finally(promise_handle,
                                [coroutine_manager = ctx.context.coroutine_manager, task, main_L]
                                (const ScriptTable& result)
                                {
                                    sol::object luaResult = ScriptTable::ToSolObject(main_L, result);
                                    coroutine_manager->ResumeTask(task, luaResult);
                                }
                            );

                            switch(type)
                            {
                                case 1: ctx.context.coroutine_manager->WaitForSeconds(task, timeout); break;
                                case 2: ctx.context.coroutine_manager->WaitForManual(task); break;
                                default: ctx.context.coroutine_manager->YieldTask(task); break;
                            }
                            return lua_yield(L, 0);
                        }

                        if (type != 1)
                            AE_CORE_WARN("[Await] Invalid promise handle={0}, yielding one frame", handle);
                        ctx.context.coroutine_manager->WaitForSeconds(task, type == 1 ? timeout : 0.0f);
                        return lua_yield(L, 0);
                    )
                ),

               AE_REFLECT("Sleep",
                    AE_MAKE_LAMBDA((), (Type& ctx, float seconds, sol::this_state ts), sol::table,
                        return MakeAwaitable(sol::state_view(ts), Handle<Promise>::MakeInvalid().Blend(), 1, seconds);
                    )
                )
            );
        }
    };

    struct PromiseBinding
    {
        using Type = PromiseContext;
        static constexpr const char* get_name() { return "PromiseContext"; }

        static constexpr auto get_methods()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("Race",
                    AE_MAKE_LAMBDA((), (Type& ctx, sol::variadic_args tables, sol::this_state ts), sol::table,
                        std::vector<Handle<Promise>> promises;
                        uint32_t final_type = 2; 
                        float final_timeout = std::numeric_limits<float>::max();

                        for (auto t : tables)
                        {
                            if (!t.is<sol::table>()) continue;
                            sol::table tbl = t.as<sol::table>();
                            uint64_t blend = tbl.get<U64>(1).value;
                            uint32_t type = tbl[2];
                            float timeout = tbl[3];
                            auto h = Handle<Promise>::FromBlend(blend);
                            if (!h.IsValid()) continue;
                            promises.push_back(h);
                            if (type == 1) 
                            {
                                final_type = 1;
                                final_timeout = std::min(final_timeout, timeout);
                            }
                        }

                        if (final_type == 1 && final_timeout == std::numeric_limits<float>::max())
                            final_timeout = 0.0f;

                        Handle<Promise> race = ctx.context.promise_manager->Race(std::move(promises));
                        return CoroutineBinding::MakeAwaitable(sol::state_view(ts), race.Blend(), final_type, final_timeout);
                    )
                ),

                AE_REFLECT("All",
                    AE_MAKE_LAMBDA((), (Type& ctx, sol::variadic_args tables, sol::this_state ts), sol::table,
                        std::vector<Handle<Promise>> promises;
                        uint32_t final_type = 2;
                        float final_timeout = std::numeric_limits<float>::max();

                        for (auto t : tables)
                        {
                            if (!t.is<sol::table>()) continue;
                            sol::table tbl = t.as<sol::table>();
                            uint64_t blend = tbl.get<U64>(1).value;
                            uint32_t type = tbl[2];
                            float timeout = tbl[3];
                            auto h = Handle<Promise>::FromBlend(blend);
                            if (!h.IsValid()) continue;
                            promises.push_back(h);
                            if (type == 1)
                            {
                                final_type = 1;
                                final_timeout = std::min(final_timeout, timeout);
                            }
                        }

                        if (final_type == 1 && final_timeout == std::numeric_limits<float>::max())
                            final_timeout = 0.0f;

                        Handle<Promise> all = ctx.context.promise_manager->All(std::move(promises));
                        return CoroutineBinding::MakeAwaitable(sol::state_view(ts), all.Blend(), final_type, final_timeout);
                    )
                )
            );
        }
    };

    struct EventBinding
    {
        using Type = EventContext;
        static constexpr const char* get_name() { return "EventContext"; }

        static sol::table Base(Type& ctx, const std::string& name, uint32_t type, float timeout, sol::this_state ts)
        {
            Handle<Promise> promise = ctx.context.promise_manager->CreatePromise();
            uint64_t blend = promise.Blend();
            Handle<EventListener> listener = ctx.context.event_manager->CreateListener(name,
                [pm = ctx.context.promise_manager, promise](const ScriptTable& args) -> bool
                {
                    pm->Resolve(promise, args);
                    return false;
                }
            );
            ctx.context.promise_manager->Finally(promise,
                [em = ctx.context.event_manager, listener](const ScriptTable& result)
                {
                    em->DestroyListener(listener);
                }
            );
            return CoroutineBinding::MakeAwaitable(sol::state_view(ts), blend, type, timeout);
        }

        static constexpr auto get_methods()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("Fire",
                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& name, sol::variadic_args args), void,
                        std::vector<sol::object> collected(args.begin(), args.end());
                        ctx.context.event_manager->FireEvent(name, collected);
                    )
                ),
                AE_REFLECT("Listen",
                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& name, sol::main_protected_function callback), U64,
                        auto handle = ctx.context.event_manager->CreateListener(name, callback, ctx.context.handle);
                        return U64(handle.Blend());
                    )
                ),
                AE_REFLECT("Unlisten",
                    AE_MAKE_LAMBDA((), (Type& ctx, U64 handle), void,
                        ctx.context.event_manager->DestroyListener(Handle<EventListener>::FromBlend(handle.value));
                    )
                ),
                AE_REFLECT("OnEvent",
                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& name, sol::this_state ts), sol::table,
                        return Base(ctx, name, 2, 0.0f, ts);
                    ),
                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& name, float timeout, sol::this_state ts), sol::table,
                        return Base(ctx, name, 1, timeout, ts);
                    )
                )
            );
        }
    };

    struct JobBinding
    {
        using Type = JobContext;
        static constexpr const char* get_name() { return "JobContext"; }

        static sol::table Base(Type& ctx, uint32_t idx, uint32_t type, float timeout, sol::variadic_args args, sol::this_state ts)
        {
            if (idx >= ctx.context.list_size)
            {
                AE_CORE_ERROR("Native index {0} not found", idx);
                return CoroutineBinding::MakeAwaitable(sol::state_view(ts), Handle<Promise>::MakeInvalid().Blend(), type, timeout);
            }
            auto func = ctx.context.native_list[idx].native;

            std::vector<ScriptTable> collected;
            for (auto arg : args)
                collected.push_back(ScriptTable::FromSolObject(arg));
            ScriptTable input = ScriptTable::Make(collected);
            auto output = CreateRef<ScriptTable>();
            Handle<Promise> promise = ctx.context.promise_manager->CreatePromise();
            ctx.context.jobsys->SubmitJob(
                [func, input = std::move(input), output]() mutable
                {
                    *output = func(input);
                },
                [pm = ctx.context.promise_manager, promise, output]()
                {
                    pm->Resolve(promise, std::move(*output));
                }
            );

            return CoroutineBinding::MakeAwaitable(sol::state_view(ts), promise.Blend(), type, timeout);
        }

        static constexpr auto get_methods()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("Run",
                    AE_MAKE_LAMBDA((), (Type& ctx, uint32_t idx, sol::variadic_args args, sol::this_state ts), sol::table,
                        return Base(ctx, idx, 2, 0.0f, args, ts);
                    )
                ),
                AE_REFLECT("RunTimeout",
                    AE_MAKE_LAMBDA((), (Type& ctx, uint32_t idx, float timeout, sol::variadic_args args, sol::this_state ts), sol::table,
                        return Base(ctx, idx, 1, timeout, args, ts);
                    )
                )
            );
        }
    };
}