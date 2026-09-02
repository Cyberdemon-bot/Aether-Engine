#pragma once

#include <vector>
#include <concepts>
#include <utility>
#include <cstdint>
#include "Aether/Core/Base.h"
#include "Aether/Container/Handle.h"

namespace Aether {

    template<typename HandleType, typename DataType>
    class ResourcePool
    {
    public:
        struct SparseSlot
        {
            uint32_t dense_index = 0;
            uint32_t generation = 0;
        };

        ResourcePool() = default;
        ~ResourcePool() = default;

        ResourcePool(ResourcePool&&) noexcept = default;
        ResourcePool& operator=(ResourcePool&&) noexcept = default;

        void Init(size_t reserve_capacity = 32)
        {
            m_Dense.reserve(reserve_capacity);
            m_DenseToSparse.reserve(reserve_capacity);
            m_Sparse.reserve(reserve_capacity);
            m_FreeList.reserve(reserve_capacity);
        }

        void Shutdown()
        {
            Clear();
            m_Dense.shrink_to_fit();
            m_DenseToSparse.shrink_to_fit();
            m_Sparse.shrink_to_fit();
            m_FreeList.shrink_to_fit();
        }

        void Clear()
        {
            m_Dense.clear();
            m_DenseToSparse.clear();
            m_Sparse.clear();
            m_FreeList.clear();
        }

        uint32_t GetSize() const { return static_cast<uint32_t>(m_Dense.size()); }
        uint32_t GetSparseSize() const { return static_cast<uint32_t>(m_Sparse.size()); }

        template<typename... Args>
        HandleType CreateResource(Args&&... args)
        {
            uint32_t sparse_index;
            if (!m_FreeList.empty())
            {
                sparse_index = m_FreeList.back();
                m_FreeList.pop_back();
            }
            else
            {
                sparse_index = static_cast<uint32_t>(m_Sparse.size());
                m_Sparse.emplace_back();
            }

            uint32_t dense_index = static_cast<uint32_t>(m_Dense.size());

            SparseSlot& slot = m_Sparse[sparse_index];
            slot.dense_index = dense_index;

            m_Dense.emplace_back(std::forward<Args>(args)...);
            m_DenseToSparse.push_back(sparse_index);

            return HandleType{ sparse_index, slot.generation };
        }

        HandleType SaveResource(const DataType& resource)
        {
            return CreateResource(resource);
        }

        HandleType SaveResource(DataType&& resource)
        {
            return CreateResource(std::move(resource));
        }

        void DestroyResource(HandleType handle)
        {
            if (!IsValid(handle)) return;

            uint32_t sparse_index = handle.index;
            SparseSlot& slot = m_Sparse[sparse_index];
            uint32_t remove_dense = slot.dense_index;
            uint32_t last_dense = static_cast<uint32_t>(m_Dense.size()) - 1;
            uint32_t last_sparse = m_DenseToSparse[last_dense];

            if (remove_dense != last_dense)
            {
                m_Dense[remove_dense] = std::move(m_Dense[last_dense]);
                m_DenseToSparse[remove_dense] = last_sparse;
                m_Sparse[last_sparse].dense_index = remove_dense;
            }

            m_Dense.pop_back();
            m_DenseToSparse.pop_back();

            slot.generation++;
            m_FreeList.push_back(sparse_index);
        }

        DataType* GetResource(HandleType handle)
        {
            if (!IsValid(handle)) return nullptr;
            return &m_Dense[m_Sparse[handle.index].dense_index];
        }

        const DataType* GetResource(HandleType handle) const
        {
            if (!IsValid(handle)) return nullptr;
            return &m_Dense[m_Sparse[handle.index].dense_index];
        }

        bool IsValid(HandleType handle) const
        {
            if (handle.index >= m_Sparse.size()) return false;
            return m_Sparse[handle.index].generation == handle.generation;
        }

        template<typename Fn>
        requires std::invocable<Fn, DataType&>
        void Loop(Fn action)
        {
            for (auto& data : m_Dense)
            {
                action(data);
            }
        }

        template<typename Fn>
        requires std::invocable<Fn, const DataType&>
        void Loop(Fn action) const
        {
            for (const auto& data : m_Dense)
            {
                action(data);
            }
        }

        DataType* Data() { return m_Dense.data(); }
        const DataType* Data() const { return m_Dense.data(); }

    private:
        ResourcePool& operator=(const ResourcePool&) = delete;
        ResourcePool(const ResourcePool&) = delete;

        std::vector<DataType> m_Dense;           
        std::vector<uint32_t> m_DenseToSparse; 
        std::vector<SparseSlot> m_Sparse;     
        std::vector<uint32_t> m_FreeList;
    };
}