#include "Aether/Core/EntryPoint.h"
#include "PBRLayer.h"
#include "GameLayer.h"

class Sandbox : public Aether::Application {
public:
    Sandbox() { 
        PushLayer(new PBRLayer()); 
        //PushLayer(new GameLayer()); 
    }
    ~Sandbox() {}
};

Aether::Application* Aether::CreateApplication() { 
    return new Sandbox(); 
}