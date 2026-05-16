#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Events/Event.h"
#include "Aether/Scene/Component.h"
#include "Aether/Core/Input.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Scene/SceneCamera.h"
#include "Aether/Physics/PhysicsSystem.h"
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

    struct CollisionBinding
    {
        using Type = CollisionData;
        using VType = glm::vec3;
        static constexpr const char* get_name() { return "CollisionData"; }

        static constexpr auto get_props()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("EntityID", 
                    AE_MAKE_LAMBDA((), (const Type& self), uint64_t,
                        return uint64_t(self.entityID);
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

    struct ScriptSelf
    {
        Scene* scene = nullptr;
        Entity entity = Null_Entity;
        InstanceSlot* slot = nullptr;
    };

    struct ScriptSelfBinding
    {
        using Type = ScriptSelf;
        static constexpr const char* get_name() { return "ScriptSelf"; }

        static constexpr auto get_props()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("Transform",
                    AE_MAKE_LAMBDA((), (const Type& self), TransformComponent*,
                        return &self.scene->GetComponent<TransformComponent>(self.entity);
                    ),
                    AE_MAKE_LAMBDA((), (Type& self, const TransformComponent& val), void,
                        self.scene->GetComponent<TransformComponent>(self.entity) = val;
                    )
                ),
                AE_REFLECT("LightConfig",
                    AE_MAKE_LAMBDA((), (const Type& self), LightParam*,
                        if (!self.scene->HasComponent<LightComponent>(self.entity)) return nullptr;
                        return &self.scene->GetComponent<LightComponent>(self.entity).Config;
                    ),
                    AE_MAKE_LAMBDA((), (Type& self, const LightParam& val), void,
                        if (!self.scene->HasComponent<LightComponent>(self.entity)) return;
                        self.scene->GetComponent<LightComponent>(self.entity).Config = val;
                    )
                ),
                AE_REFLECT("Camera",
                    AE_MAKE_LAMBDA((), (const Type& self), SceneCamera*,
                        if (!self.scene->HasComponent<CameraComponent>(self.entity)) return nullptr;
                        return &self.scene->GetComponent<CameraComponent>(self.entity).Camera;
                    ),
                    AE_MAKE_LAMBDA((), (Type& self, const SceneCamera& val), void,
                        if (!self.scene->HasComponent<CameraComponent>(self.entity)) return;
                        self.scene->GetComponent<CameraComponent>(self.entity).Camera = val;
                    )
                ),

                AE_REFLECT("IsPrimaryCamera",
                    AE_MAKE_LAMBDA((), (const Type& self), bool,
                        if (!self.scene->HasComponent<CameraComponent>(self.entity)) return false;
                        return self.scene->GetComponent<CameraComponent>(self.entity).Primary;
                    ),
                    AE_MAKE_LAMBDA((), (Type& self, bool val), void,
                        if (!self.scene->HasComponent<CameraComponent>(self.entity)) return;
                        self.scene->GetComponent<CameraComponent>(self.entity).Primary = val;
                    )
                ),
                AE_REFLECT("EntityID",
                    AE_MAKE_LAMBDA((), (const Type& self), uint64_t,
                        return uint64_t(self.scene->GetComponent<IDComponent>(self.entity).ID);
                    )
                ), 
                AE_REFLECT("ExecOrder",
                    AE_MAKE_LAMBDA((), (const Type& self), int,
                        ScriptEngine::MarkExecOrderChanged(); return self.slot->exec_order;
                    ),
                    AE_MAKE_LAMBDA((), (Type& self, int val), void,
                        self.slot->exec_order = val;
                    )
                )
            );
        }

        static constexpr auto get_methods()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("Expose",
                    AE_MAKE_LAMBDA((), (Type& self, const std::string& name, sol::protected_function func), void,
                        self.slot->exposed_funcs[name] = std::move(func);
                    )
                ),
                AE_REFLECT("Call",
                    AE_MAKE_LAMBDA((), (Type& self, uint64_t targetId, const std::string& name, sol::variadic_args args), sol::object,
                        Entity target = self.scene->FindEntity((UUID(targetId)));
                        if (!self.scene->IsValid(target)) return sol::lua_nil;
                        if (!self.scene->HasComponent<ScriptComponent>(target)) return sol::lua_nil;
                        auto& sc = self.scene->GetComponent<ScriptComponent>(target);
                        std::vector<sol::object> collected(args.begin(), args.end());
                        return ScriptEngine::CallInstanceAPI(sc.ScriptHandle, name, collected);
                    )
                ),
                AE_REFLECT("DestroyMyself",
                    AE_MAKE_LAMBDA((), (Type& self), void,
                        Entity e = static_cast<Entity>(self.entity);
                        auto& sc = self.scene->GetComponent<ScriptComponent>(e);
                        ScriptEngine::PushDestroyQueue(e, sc.ScriptHandle);
                    )
                )
            );
        }
    };

    struct SceneContext
    {
        Scene* scene;
    };

    struct SceneBinding
    {
        using Type = SceneContext;
        static constexpr const char* get_name() { return "SceneContext"; }

        static constexpr auto get_methods()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("FindByName",
                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& name), std::vector<uint64_t>,
                        auto results = ctx.scene->FindEntity(name);
                        std::vector<uint64_t> ids;
                        ids.reserve(results.size());
                        for (auto& e : results)
                            if (ctx.scene->HasComponent<ScriptComponent>(e))
                                ids.push_back(uint64_t(ctx.scene->GetComponent<IDComponent>(e).ID));
                        return ids;
                    )
                ),

                AE_REFLECT("IsValid",
                    AE_MAKE_LAMBDA((), (Type& ctx, uint64_t id), bool,
                        Entity e = ctx.scene->FindEntity(UUID(id));
                        return ctx.scene->IsValid(e);
                    )
                )
            );
        }
    };

    struct EventContext
    {
        Handle<ScriptInstance> handle;
        ScriptEventManager* event_manager = nullptr;
    };

    struct EventManagerBinding
    {
        using Type = EventContext;
        static constexpr const char* get_name() { return "EventContext"; }

        static constexpr auto get_methods()
        {
            return AE_REFLECT_LIST(
                AE_REFLECT("Fire",
                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& name, sol::variadic_args args), void,
                        std::vector<sol::object> collected(args.begin(), args.end());
                        ctx.event_manager->FireEvent(name, std::move(collected));
                    )
                ),
                AE_REFLECT("Listen",
                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& name, sol::protected_function cb), void,
                        ctx.event_manager->AddListener(ctx.handle, name, std::move(cb));
                    )
                ),
                AE_REFLECT("Unlisten",
                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& name), void,
                        ctx.event_manager->RemoveListener(ctx.handle, name);
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

    struct RaycastHitBinding
    {
        using Type = RaycastHit;
        using VType = glm::vec3;
        static constexpr const char* get_name() { return "RaycastHit"; }

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
                AE_REFLECT("HitEntity",
                    AE_MAKE_LAMBDA((), (const Type& self), uint64_t,
                        return self.HitEntityID;
                    )
                )
            );
        }
    };

    struct PhysicsContext
    {
        Scene* scene  = nullptr;
        Entity entity = Null_Entity;
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
                    AE_MAKE_LAMBDA((), (Type& self, const VType& force), void,
                        if (!self.scene->HasComponent<ColliderComponent>(self.entity)) return;
                        auto handle = self.scene->GetComponent<ColliderComponent>(self.entity).ColliderHandle;
                        PhysicsSystem::AddForce(self.scene->GetPhysicsInstance(), handle, force);
                    )
                ),

                AE_REFLECT("SetVelocity",
                    AE_MAKE_LAMBDA((), (Type& self, const VType& velocity), void,
                        if (!self.scene->HasComponent<ColliderComponent>(self.entity)) return;
                        auto handle = self.scene->GetComponent<ColliderComponent>(self.entity).ColliderHandle;
                        PhysicsSystem::SetVelocity(self.scene->GetPhysicsInstance(), handle, velocity);
                    )
                ),

                AE_REFLECT("SetGravity",
                    AE_MAKE_LAMBDA((), (Type& self, const VType& gravity), void,
                        PhysicsSystem::SetGravity(self.scene->GetPhysicsInstance(), gravity);
                    )
                ),

                AE_REFLECT("CanMove",
                    AE_MAKE_LAMBDA((), (Type& self, const PhysTransform& target), bool,
                        if (!self.scene->HasComponent<ColliderComponent>(self.entity)) return false;
                        auto handle = self.scene->GetComponent<ColliderComponent>(self.entity).ColliderHandle;
                        return PhysicsSystem::CanMove(self.scene->GetPhysicsInstance(), handle, target);
                    )
                ),

                AE_REFLECT("CastRay",
                    AE_MAKE_LAMBDA((), (Type& self, const VType& origin, const VType& direction, float distance), RaycastHit,
                        return PhysicsSystem::CastRay(self.scene->GetPhysicsInstance(), origin, direction, distance);
                    )
                ),

                AE_REFLECT("CastRayAll",
                    AE_MAKE_LAMBDA((), (Type& self, const VType& origin, const VType& direction, float distance), std::vector<RaycastHit>,
                        return PhysicsSystem::CastRayAll(self.scene->GetPhysicsInstance(), origin, direction, distance);
                    )
                )
            );
        }
    };
}