#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Events/Event.h"
#include "Aether/Scene/Component.h"
#include "Aether/Core/Input.h"
#include "Aether/Scripting/ScriptEngine.h"
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
                AE_REFLECT("EntityId", 
                    AE_MAKE_LAMBDA((), (const Type& self), uint32_t,
                        return self.entity;
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
                AE_REFLECT("EntityId",
                    AE_MAKE_LAMBDA((), (const Type& self), uint32_t,
                        return static_cast<uint32_t>(self.entity);
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
                    AE_MAKE_LAMBDA((), (Type& self, uint32_t targetId, const std::string& name, sol::variadic_args args), sol::object,
                        Entity target = static_cast<Entity>(targetId);
                        if (!self.scene->IsValid(target)) return sol::lua_nil;
                        if (!self.scene->HasComponent<ScriptComponent>(target)) return sol::lua_nil;
                        auto& sc = self.scene->GetComponent<ScriptComponent>(target);
                        std::vector<sol::object> collected(args.begin(), args.end());
                        return ScriptEngine::CallInstanceAPI(sc.ScriptHandle, name, collected);
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
                AE_REFLECT("CreateEntity",
                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& name, uint64_t bh), uint32_t,
                        Entity e = ctx.scene->CreateEntity(name);
                        Handle<BytecodeTag> bytecode = Handle<BytecodeTag>::FromBlend(bh);
                        Handle<ScriptTag> handle = ScriptEngine::CreateInstance(ctx.scene, e, bytecode);
                        if (handle.IsValid())
                        {
                            ctx.scene->AddComponent<ScriptComponent>(e, handle);
                            ScriptEngine::StartInstance(handle);
                        }
                        return static_cast<uint32_t>(e);
                    ),

                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& name, uint64_t bh, uint32_t parentId), uint32_t,
                        Entity parent = static_cast<Entity>(parentId);
                        Entity e = ctx.scene->CreateEntity(name, parent);
                        Handle<BytecodeTag> bytecode = Handle<BytecodeTag>::FromBlend(bh);
                        Handle<ScriptTag> handle = ScriptEngine::CreateInstance(ctx.scene, e, bytecode);
                        if (handle.IsValid())
                        {
                            ctx.scene->AddComponent<ScriptComponent>(e, handle);
                            ScriptEngine::StartInstance(handle);
                        }
                        return static_cast<uint32_t>(e);
                    )
                ),

                AE_REFLECT("DestroyEntity",
                    AE_MAKE_LAMBDA((), (Type& ctx, uint32_t id), void,
                        Entity e = static_cast<Entity>(id);
                        if (!ctx.scene->IsValid(e)) return;
                        if (ctx.scene->HasComponent<ScriptComponent>(e))
                        {
                            auto& sc = ctx.scene->GetComponent<ScriptComponent>(e);
                            if (sc.ScriptHandle.IsValid())
                                ScriptEngine::PushDestroyQueue(e, sc.ScriptHandle);
                        }
                    )
                ),

                AE_REFLECT("DestroyHierarchy",
                    AE_MAKE_LAMBDA((), (Type& ctx, uint32_t id), void,
                        Entity e = static_cast<Entity>(id);
                        if (!ctx.scene->IsValid(e)) return;
                        if (ctx.scene->HasComponent<ScriptComponent>(e))
                        {
                            auto& sc = ctx.scene->GetComponent<ScriptComponent>(e);
                            if (sc.ScriptHandle.IsValid())
                                ScriptEngine::DestroyInstance(sc.ScriptHandle);
                        }
                        ctx.scene->DestroyHierarchy(e);
                    )
                ),

                AE_REFLECT("FindByName",
                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& name), uint32_t,
                        auto results = ctx.scene->FindEntity(name);
                        if (results.empty()) return static_cast<uint32_t>(Null_Entity);
                        return static_cast<uint32_t>(results[0]);
                    )
                ),

                AE_REFLECT("IsValid",
                    AE_MAKE_LAMBDA((), (Type& ctx, uint32_t id), bool,
                        Entity e = static_cast<Entity>(id);
                        return ctx.scene->IsValid(e);
                    )
                ),

                AE_REFLECT("LoadScript",
                    AE_MAKE_LAMBDA((), (Type& ctx, const std::string& path), uint64_t,
                        return ScriptEngine::LoadScript(path).Blend();
                    )
                )
            );
        }
    };

    struct EventContext
    {
        Handle<ScriptTag> handle;
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
}