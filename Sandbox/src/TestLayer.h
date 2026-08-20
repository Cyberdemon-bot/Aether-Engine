#pragma once

#include "Aether.h"

class TestLayer : public Aether::Layer
{
public:
    TestLayer();
    virtual ~TestLayer() = default;

    virtual void Attach() override;
    virtual void Detach() override;
    virtual void OnUpdate(Aether::Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Aether::Event& event) override;
};