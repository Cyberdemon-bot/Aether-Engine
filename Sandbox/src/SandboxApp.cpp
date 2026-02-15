#include "Aether/Core/EntryPoint.h"
#include "TestLayer.h"

class Sandbox : public Aether::Application {
public:
    Sandbox() { 
        PushLayer(new TestLayer());
    }
    ~Sandbox() {}
};

Aether::Application* Aether::CreateApplication() { 
    return new Sandbox(); 
}