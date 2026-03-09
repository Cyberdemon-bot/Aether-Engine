#include "Aether/Assets/Asset.h"
#include "Aether/Core/UUID.h"
#include <unordered_map>

namespace Aether {

    class AETHER_API AssetManager
    {
    public:
        static void Init();
        static void Shutdown();
        static void RegisterResource(Ref<Asset> asset, UUID id);

        template<typename T>
        static Ref<T> GetResource(UUID id)
        {
            auto& instance = GetInstance();
            auto it = instance.m_Registry.find(id);
            if (it != instance.m_Registry.end() && it->second == T::GetType()) 
                return std::static_pointer_cast<T>(instance.m_Resources[id]);
            return nullptr;
        }
        
    private:
        AssetManager() = default;
        static AssetManager& GetInstance();
        std::unordered_map<UUID, AssetType> m_Registry;
        std::unordered_map<UUID, Ref<Asset>> m_Resources;
    };
}