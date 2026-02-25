#include "Aether/Core/EntryPoint.h"
#include "MainGameLayer.h"
#include "GameLayer.h"

class Sandbox : public Aether::Application {
public:
    Sandbox() { 
        //PushLayer(new GameLayer());
        PushLayer(new MainGameLayer());
    }
    ~Sandbox() {}
};

Aether::Application* Aether::CreateApplication() { 
    return new Sandbox(); 
}