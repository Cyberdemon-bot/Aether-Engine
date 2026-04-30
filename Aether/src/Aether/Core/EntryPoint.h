#pragma once
#include "Application.h"

extern Aether::Application* Aether::CreateApplication();

int main(int argc, char** argv)
{
    Aether::Log::Init();
    auto app = Aether::CreateApplication();
    app->Run();
    delete app;
}
