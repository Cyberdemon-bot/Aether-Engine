#pragma once

#include <vector>
#include "Aether/Scene/TComponentInfo.h"
#include "Aether/Container/Handle.h"
#include "Aether/Container/StringTable.h"

namespace Aether {
    class ComponentRegistry
    {
    public:
        Handle<TComponentInfo> RegisterComponent(const TComponentInfo& info);

        template<typename T>
        Handle<TComponentInfo> RegisterComponent()
        {
            return RegisterComponent(ComponentInfoFactory<T>::Create());
        }

        const TComponentInfo* GetComponentInfo(Handle<TComponentInfo> handle) const;
        const TComponentInfo* GetComponentInfo(std::string_view name) const;
    private:
        StringTable<Handle<TComponentInfo>, TComponentInfo> m_Table;
    };
}