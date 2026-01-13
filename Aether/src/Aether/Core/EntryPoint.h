#pragma once
#include "Application.h"

extern Aether::Application* Aether::CreateApplication();

int main(int argc, char** argv)
{
    Aether::Log::Init();
    AE_CORE_WARN("Initialized Log!");
    auto app = Aether::CreateApplication();
    app->Run();
    delete app;
}
