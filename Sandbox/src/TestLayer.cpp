#include "TestLayer.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>

struct TestTransform 
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct TestHealth 
{
    int hp = 100;
    int maxHp = 100;

    TestHealth() : hp(100), maxHp(100) {}
    TestHealth(int h, int m) : hp(h), maxHp(m) {}
    ~TestHealth() { hp = 0; }
};

void RunArchetypeTest()
{
    AE_CORE_INFO("========== STARTING ARCHETYPE TEST ==========");

    // 1. Dùng Factory tạo metadata cho các kiểu nguyên thủy (int, float) lẫn struct tự chế
    std::vector<Aether::TComponentInfo> components = {
        Aether::ComponentInfoFactory<uint32_t>::Create(),      // Primitive ID
        Aether::ComponentInfoFactory<float>::Create(),         // Primitive Weight
        Aether::ComponentInfoFactory<TestTransform>::Create(), // POD Struct
        Aether::ComponentInfoFactory<TestHealth>::Create()     // Non-trivial Struct
    };

    // 2. Khởi tạo Archetype với ChunkSizeBytes nhỏ (1024 bytes) để dễ kích hoạt cấp phát nhiều Chunk
    constexpr size_t TestChunkSize = 1024;
    constexpr size_t MaxComponents = 16;
    Aether::Archetype<TestChunkSize, MaxComponents> archetype(components);

    // 3. Test CreateElement: Tạo liên tục phần tử để phân bổ trên nhiều Chunk
    constexpr size_t TotalElementsToCreate = 50;
    std::vector<uint32_t> createdIndices;
    createdIndices.reserve(TotalElementsToCreate);

    for (size_t i = 0; i < TotalElementsToCreate; ++i)
    {
        uint32_t newIdx = 0;
        bool success = archetype.CreateElement(newIdx);
        
        assert(success && "CreateElement failed!");
        assert(newIdx == i && "Global index mismatch!");
        
        createdIndices.push_back(newIdx);
    }
    AE_CORE_INFO("[Test Passed] Successfully created {0} elements across chunks.", TotalElementsToCreate);

    // 4. Test DestroyElement (Trường hợp Cross-Chunk Swap-and-Pop)
    // Xóa 1 phần tử ở vị trí giữa (ví dụ: index 5)
    uint32_t targetIdx = 5;
    Aether::Archetype<TestChunkSize, MaxComponents>::SwapResult swapResult{};
    
    bool destroySuccess = archetype.DestroyElement(targetIdx, swapResult);

    assert(destroySuccess && "DestroyElement failed!");
    assert(swapResult.swapped == true && "Deleting middle element MUST trigger a swap!");
    assert(swapResult.movedElementIdx == (TotalElementsToCreate - 1) && "Moved element MUST be the last global element!");

    AE_CORE_INFO("[Test Passed] Destroyed element at index {0}. Element {1} was swapped into its place.", 
                 targetIdx, swapResult.movedElementIdx);

    // 5. Test DestroyElement (Trường hợp POP thuần túy)
    // Xóa phần tử cuối cùng hiện tại (index lúc này là TotalElementsToCreate - 2)
    uint32_t lastIdx = TotalElementsToCreate - 2;
    destroySuccess = archetype.DestroyElement(lastIdx, swapResult);

    assert(destroySuccess && "DestroyElement (POP) failed!");
    assert(swapResult.swapped == false && "Deleting the last element MUST NOT trigger a swap!");

    AE_CORE_INFO("[Test Passed] Pop-destroyed last element at index {0} without swapping.", lastIdx);

    // 6. Test DestroyElement với Index không hợp lệ (Out of bounds)
    bool invalidDestroy = archetype.DestroyElement(9999, swapResult);
    assert(!invalidDestroy && "DestroyElement MUST fail when index is out of bounds!");

    AE_CORE_INFO("[Test Passed] Out-of-bounds safety check passed.");
    AE_CORE_INFO("========== ARCHETYPE TEST PASSED SUCCESSFULLY ==========");
}


TestLayer::TestLayer()
    : Layer("Main Game")
{
   
}

void TestLayer::Attach()
{
    RunArchetypeTest();
}

void TestLayer::Detach() 
{
}

void TestLayer::OnUpdate(Aether::Timestep ts)
{
}

void TestLayer::OnImGuiRender() 
{
}

void TestLayer::OnEvent(Aether::Event& event)
{
}