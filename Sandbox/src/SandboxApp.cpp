#include "Aether/Core/EntryPoint.h"
//#include "TestLayer.h"
#include "GameLayer.h"

class Sandbox : public Aether::Application {
public:
    Sandbox() { 
        PushLayer(new GameLayer());
    }
    ~Sandbox() {}
};

Aether::Application* Aether::CreateApplication() { 
    return new Sandbox(); 
}