#include "Aether/Core/EntryPoint.h"
#include "MainGameLayer.h"
#include "LabLayer.h"
#include "TestLayer.h"

class Sandbox : public Aether::Application 
{
public:
    Sandbox() 
    { 
        SetIcon("assets/textures/wood.jpg");
        //PushLayer(new LabLayer());
        PushLayer(new MainGameLayer());
        //PushLayer(new TestLayer());
    }
    ~Sandbox() {}
};

Aether::Application* Aether::CreateApplication() { 
    return new Sandbox(); 
}