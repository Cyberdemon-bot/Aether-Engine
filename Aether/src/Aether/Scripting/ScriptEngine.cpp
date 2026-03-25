#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Core/Log.h"

#if 0
namespace Aether 
{
    sol::meta_function ScriptEngine::OpNameToMeta(std::string_view name)
    {
        if (name == "ADD") return sol::meta_function::addition;
        if (name == "SUB") return sol::meta_function::subtraction;
        if (name == "MUL") return sol::meta_function::multiplication;
        if (name == "DIV") return sol::meta_function::division;
        if (name == "EQ")  return sol::meta_function::equal_to;
        if (name == "LT")  return sol::meta_function::less_than;
        if (name == "LE")  return sol::meta_function::less_than_or_equal_to;
        if (name == "MOD") return sol::meta_function::modulus;
        if (name == "UNM") return sol::meta_function::unary_minus;
        AE_CORE_WARN("[ScriptEngine] Unknown op name: {0}", name);
        return sol::meta_function::addition;
    }

    template<typename UserType, typename Tuple, std::size_t... Is>
    void ScriptEngine::RegisterMembers(UserType& ut, const Tuple& members, std::index_sequence<Is...>)
    {
            ([&] {
                const auto& entry = std::get<Is>(members);
                const char* name = std::get<0>(entry);
                auto ptr = std::get<1>(entry);
                bool readOnly = std::get<2>(entry);

                if (readOnly) ut[name] = sol::readonly(ptr);
                else ut[name] = ptr;
            }(), ...);
    }

    template<std::size_t Start, std::size_t Count, typename UserType, typename Tuple, std::size_t... Is>
    void ScriptEngine::RegisterOpGroup(UserType& ut, const Tuple& ops, std::index_sequence<Is...>)
    {
        const char* opName = std::get<0>(std::get<Start>(ops));
        auto meta = OpNameToMeta(opName);

        if constexpr (Count == 1) ut[meta] = std::get<1>(std::get<Start>(ops));
        else ut[meta] = sol::overload(std::get<1>(std::get<Start + Is>(ops))...);
    }

    template<std::size_t I = 0, typename UserType, typename Tuple>
    void ScriptEngine::RegisterOpsSorted(UserType& ut, const Tuple& ops)
    {
        if constexpr (I < std::tuple_size_v<Tuple>)
        {
            constexpr std::string_view name = std::get<0>(std::get<I>(ops));
            constexpr size_t count = []<size_t... Js>(std::index_sequence<Js...>) constexpr -> size_t
            {
                size_t n = 1;
                bool done = false;
                ((done ? (void)0 : (std::string_view(std::get<0>(std::get<I + 1 + Js>(ops))) == name ? (void)++n : (void)(done = true))), ...);
                return n;
            }(std::make_index_sequence<std::tuple_size_v<Tuple> - I - 1>{});
            RegisterOpGroup<I, count>(ut, ops, std::make_index_sequence<count>{});
            RegisterOpsSorted<I + count>(ut, ops);
        }   
    }

    template<typename T>
    void ScriptEngine::RegisterType()
    {
        auto ut = s_Lua.new_usertype<T>(
            T::get_name(), 
            sol::constructor<T()>()
        );

        if constexpr (requires { T::reflect() })
        {
            constexpr auto members = T::reflect();
            RegisterMembers(ut, members, std::make_index_sequence<std::tuple_size_v<decltype(members)>>{});
        }

        if constexpr (requires { T::script_op(); })
        {
            constexpr auto ops = T::script_op();
            RegisterOpsSorted(ut, ops);
        }
    }

    template<typename... Ts>
    void ScriptEngine::RegisterTypes()
    {
        (RegisterType<Ts>(), ...);
    }

    void ScriptEngine::BindAPI(Scene* scene)
    {
    }

    // bool ScriptEngine::CallMethod(ScriptComponent& comp, const char* method, Args&&... args)
    // {
        
    // }

    void ScriptEngine::LoadScript(Scene* scene, Entity entity)
    {
        
    }

    void ScriptEngine::Init()
    {

    }

    void ScriptEngine::Shutdown()
    {

    }

    void ScriptEngine::Create(Scene* scene, Entity entity)
    {

    }

    void ScriptEngine::Update(Scene* scene, Timestep ts)
    {

    }

    void ScriptEngine::Destroy(Scene* scene, Entity entity)
    {

    }

    void ScriptEngine::OnEvent(Scene* scene, Event& event)
    {

    }

    void ScriptEngine::ReloadScript(Scene* scene, Entity entity)
    {

    }

    void ScriptEngine::LoadScript(Scene* scene, Entity entity)
    {

    }

    void ScriptEngine::UnloadScript(Scene* scene, Entity entity)
    {

    }

    void ScriptEngine::BindAPI(Scene* scene)
    {

    }

    void ScriptEngine::CallMethod(sol::table& instance, const char* method, auto&&... args)
    {

    }
}
#endif