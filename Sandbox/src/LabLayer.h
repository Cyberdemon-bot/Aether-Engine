#pragma once
#include "Aether.h"
#include <glm/glm.hpp> 


class LabLayer : public Aether::Layer
{
public:
    LabLayer();
    virtual ~LabLayer() = default;

    virtual void Detach() override;
    virtual void Attach() override;
    virtual void Update(Aether::Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Aether::Event& event) override;
private: 
    Aether::Ref<Aether::Texture2D> mTexture;
};