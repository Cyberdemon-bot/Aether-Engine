#include "Aether/Core/EntryPoint.h"
#include "DemoLayer.h"
#include "LabLayer.h"

class Sandbox : public Aether::Application {
public:
    Sandbox() { 
        //PushLayer(new DemoLayer()); 
        PushLayer(new LabLayer());
    }
    ~Sandbox() {}
};

Aether::Application* Aether::CreateApplication() { 
    return new Sandbox(); 
}