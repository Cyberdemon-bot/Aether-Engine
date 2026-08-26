#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp"
#include "Aether/Core/Base.h"
#include "Aether/Container/Handle.h"
#include "Aether/Container/ResourcePool.h"

namespace Aether {  

    struct Transform
    {
        glm::vec3 translation = {0.0f, 0.0f, 0.0f};
        glm::quat rotation = {1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale = {1.0f, 1.0f, 1.0f};
    };

    struct HierarchyNode
    {
        Transform local;
        glm::mat4 world = glm::mat4(1.0f);  

        uint32_t parent = 0;
        uint32_t fchild = 0;
        uint32_t nsibling = 0;
        uint32_t level = 0;

        uint8_t tag = 0; //4bit for parent, fchild, nsibling existence + 1bit for dirty

        inline void mark_dirty() { SetBit(tag, true, 0); }
        inline void reset_dirty() { SetBit(tag, false, 0); }

        inline void set_parent(uint32_t val) { SetBit(tag, true, 1); parent = val; }
        inline void set_fchild(uint32_t val) { SetBit(tag, true, 2); fchild = val; }
        inline void set_nsibling(uint32_t val) { SetBit(tag, true, 3); nsibling = val; }

        inline void reset_parent() { SetBit(tag, false, 1); }
        inline void reset_fchild() { SetBit(tag, false, 2); }
        inline void reset_nsibling() { SetBit(tag, false, 3); }

        inline bool has_parent() const { return HasBit(tag, 1); }
        inline bool has_fchild() const { return HasBit(tag, 2); }
        inline bool has_nsibling() const { return HasBit(tag, 3); }
    };

    struct NodeMeta
    {
        uint32_t level = 0;
        uint32_t index = 0;
        bool valid = false;
    };

    class HierarchySystem
    {
    public:
        void Init();
        void Shutdown();
        void OnUpdate();    

        Handle<HierarchyNode> CreateNode();
        void ReparentAndDestroyNode(Handle<HierarchyNode> handle);
        void DestroySubtree(Handle<HierarchyNode> handle);

        void SetParent(Handle<HierarchyNode> node, Handle<HierarchyNode> parent, bool keepWorldTransform = true);
        void DetachFromParent(Handle<HierarchyNode> node, bool keepWorldTransform = true);
        void DetachChildren(Handle<HierarchyNode> parent, bool keepWorldTransform = true);

        void SetTransform(Handle<HierarchyNode> handle, const Transform& transform);
        const Transform* GetTransform(Handle<HierarchyNode> handle);
    private:
        
    };
}