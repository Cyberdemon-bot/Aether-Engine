#include "Aether/Core/EntryPoint.h"
#include "Game.h"

class GameApp : public Aether::Application 
{
public:
    GameApp() 
    { 
        PushLayer(new Game());
    }
    ~GameApp() {}
};

Aether::Application* Aether::CreateApplication() 
{ 
    return new GameApp(); 
}