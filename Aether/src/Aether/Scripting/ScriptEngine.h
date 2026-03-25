#include "Aether/Scene/Scene.h"
#include "Aether/Events/Event.h"
#include <sol/sol.hpp>
#include "Aether/Scene/Component.h"

namespace Aether 
{
    class ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();

        static void CreateInstance(Scene* scene, Entity entity);
        static void UpdateInstance(Scene* scene, Timestep ts);
        static void DestroyInstance(Scene* scene, Entity entity);
        static void OnEvent(Scene* scene, Event& event);
        static void ReloadScript(Scene* scene, Entity entity);

    private:
        static void LoadScript(Scene* scene, Entity entity);
        static void UnloadScript(Scene* scene, Entity entity);
        static void BindAPI(Scene* scene);     
        //static void CallMethod(ScriptComponent& comp, const char* method, Args&&... args)

        template<typename T>
        static void RegisterType();
    
        template<typename... Ts>
        static void RegisterTypes();
    
        template<typename UserType, typename Tuple, std::size_t... Is>
        static void RegisterMembers(UserType& ut, const Tuple& members, std::index_sequence<Is...>);
    
        template<std::size_t I = 0, typename UserType, typename Tuple>
        static void RegisterOpsSorted(UserType& ut, const Tuple& ops);
    
        template<std::size_t Start, std::size_t Count, typename UserType, typename Tuple, std::size_t... Is>
        static void RegisterOpGroup(UserType& ut, const Tuple& ops, std::index_sequence<Is...>);
    
        static sol::meta_function OpNameToMeta(std::string_view name);
        static inline sol::state s_Lua;
    };
}