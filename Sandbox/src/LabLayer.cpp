#include "LabLayer.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


LabLayer::LabLayer() : Layer("Lab layer") {}

void LabLayer::Attach()
{
    ImGuiContext* IGContext = Aether::ImGuiLayer::GetContext();
    if (IGContext) ImGui::SetCurrentContext(IGContext);

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("assets/models/robot.glb", 0);

    const aiTexture* texture = scene->mTextures[0];
    size_t size = (texture->mHeight == 0) ? texture->mWidth : texture->mHeight * texture->mWidth * 4;

    mTexture = Aether::Texture2D::Create(texture->pcData, size);
}

void LabLayer::OnImGuiRender()
{
    ImGui::Begin("assimp tex test");

    ImGui::Text("Size %d x %d", mTexture->GetWidth(), mTexture->GetHeight());

    float availWidth = ImGui::GetContentRegionAvail().x;
    float aspect = (float)mTexture->GetHeight() / (float)mTexture->GetWidth();

    ImGui::Image((void*)(uintptr_t)mTexture->GetRendererID(), ImVec2(availWidth, availWidth * aspect));

    ImGui::End();
}

void LabLayer::Update(Aether::Timestep ts)
{

}

void LabLayer::Detach()
{

}

void LabLayer::OnEvent(Aether::Event& event)
{

}