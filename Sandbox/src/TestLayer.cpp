#include "TestLayer.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>

void TestFileSystemIntegration()
{
    using namespace Aether;
    // 1. Lấy con trỏ FileSystem từ ServiceManager theo cú pháp yêu cầu
    auto* fs = Aether::ServiceManager::GetService<Aether::FileSystem>();
    assert(fs != nullptr && "FileSystem Service chưa được đăng ký hoặc khởi tạo!");

    std::string testRoot = "assets/textures";
    std::string testFile = "wood.jpg";
    std::string virtualPath = "textures/wood.jpg";
    auto looseProvider = std::make_shared<Aether::LooseFileProvider>(testRoot);

    fs->RegisterPath(virtualPath);
    fs->CommitRegistry();
    fs->Mount("textures/", looseProvider, 0);
    std::filesystem::path rawPath = std::filesystem::path(testRoot) / testFile;
    std::ifstream rawFile(rawPath, std::ios::binary | std::ios::ate);
    
    if (!rawFile.is_open())
    {
        std::cerr << "[Test Error] Không thể mở file gốc bằng STD tại: " << rawPath.string() << " để làm mẫu so sánh.\n";
        return;
    }

    std::streamsize rawSize = rawFile.tellg();
    rawFile.seekg(0, std::ios::beg);

    std::vector<uint8_t> expectedBytes(static_cast<size_t>(rawSize));
    if (rawSize > 0)
    {
        rawFile.read(reinterpret_cast<char*>(expectedBytes.data()), rawSize);
    }
    rawFile.close();

    std::cout << "[Test Log] Đã tải file mẫu vật lý bằng STD. Kích thước: " << rawSize << " bytes.\n";

    Aether::Handle<Aether::FileData> handle = fs->Open(virtualPath);
    Aether::Handle<Aether::FileData> handle2 = fs->Open("textures/wood.jpg"_hash);
    if (handle.Blend() == handle2.Blend())
    {
        std::cerr << "[Test Success] Hash thành công " << virtualPath << "\n";
    }
    else
    {   
        std::cerr << "[Test Success] Hash thất bại " << virtualPath << "\n";
    }

    if (!fs->IsValid(handle))
    {
        std::cerr << "[Test Failed] FileSystem không thể mở đường dẫn ảo: " << virtualPath << "\n";
        return;
    }

    Aether::FileData vfsData = fs->GetBytes(handle);
    bool testPassed = true;

    if (vfsData.size != expectedBytes.size())
    {
        std::cerr << "[Test Failed] Sai lệch kích thước file! VFS: " << vfsData.size 
                  << " bytes | STD: " << expectedBytes.size() << " bytes.\n";
        testPassed = false;
    }
    else
    {
        for (size_t i = 0; i < expectedBytes.size(); ++i)
        {
            if (vfsData.bytes[i] != expectedBytes[i])
            {
                std::cerr << "[Test Failed] Sai lệch dữ liệu tại byte thứ: " << i 
                          << " | VFS: " << (int)vfsData.bytes[i] 
                          << " | STD: " << (int)expectedBytes[i] << "\n";
                testPassed = false;
                break;
            }
        }
    }
    fs->Close(handle);
    if (testPassed)
    {
        std::cout << "[Test Success] HỆ THỐNG VFS HOẠT ĐỘNG CHÍNH XÁC 100%! Dữ liệu trùng khớp hoàn toàn.\n";
    }
    else
    {
        std::cout << "[Test Failed] Thử nghiệm thất bại. Vui lòng kiểm tra lại luồng đọc dữ liệu.\n";
    }
}


TestLayer::TestLayer()
    : Layer("Main Game")
{
   
}

void TestLayer::Attach()
{
    TestFileSystemIntegration();
}

void TestLayer::Detach() 
{
}

void TestLayer::Update(Aether::Timestep ts)
{
}

void TestLayer::OnImGuiRender() 
{
}

void TestLayer::OnEvent(Aether::Event& event)
{
}