#include "aepch.h"
#include "Aether/Scene/Archetype.h"
#include "Aether/Scene/ComponentRegistry.h"

namespace Aether {
    Handle<TComponentInfo> ComponentRegistry::RegisterComponent(const TComponentInfo& info)
    {
        auto handle = m_Table.SafeCreate(info.name, info);
        if (handle.Blend() == Handle<TComponentInfo>::Null().Blend()) AE_CORE_WARN("[ComponentRegistry] Failed to register component '{0}': Name already exists", info.name);
        auto* data = m_Table.GetData(handle); data->id = handle;
        return handle;
    }
    
    const TComponentInfo* ComponentRegistry::GetComponentInfo(Handle<TComponentInfo> handle) const
    {
        const TComponentInfo* info = m_Table.GetData(handle);
        return info;
    }
    
    const TComponentInfo* ComponentRegistry::GetComponentInfo(std::string_view name) const
    {
        auto handle = m_Table.Search(name);
        if (handle.Blend() == Handle<TComponentInfo>::Null().Blend()) return nullptr;
        const TComponentInfo* info = m_Table.GetData(handle);
        return info;
    }
}