#include "Aether/Core/EntryPoint.h"
#include "DemoLayer.h"

class Sandbox : public Aether::Application {
public:
    Sandbox() { 
        PushLayer(new DemoLayer()); 
    }
    ~Sandbox() {}
};

Aether::Application* Aether::CreateApplication() { 
    return new Sandbox(); 
}