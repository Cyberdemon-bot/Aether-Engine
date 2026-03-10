#include "Aether/Core/EntryPoint.h"
#include "MainGameLayer.h"
#include "LabLayer.h"

class Sandbox : public Aether::Application {
public:
    Sandbox() { 
        //PushLayer(new LabLayer());
        PushLayer(new MainGameLayer());
    }
    ~Sandbox() {}
};

Aether::Application* Aether::CreateApplication() { 
    return new Sandbox(); 
}