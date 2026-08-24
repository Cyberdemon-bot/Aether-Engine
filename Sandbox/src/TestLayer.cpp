#include "TestLayer.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>


TestLayer::TestLayer()
    : Layer("Main Game")
{
   
}

void TestLayer::Attach()
{
}

void TestLayer::Detach() 
{
}

void TestLayer::OnUpdate(Aether::Timestep ts)
{
}

void TestLayer::OnImGuiRender() 
{
}

void TestLayer::OnEvent(Aether::Event& event)
{
}