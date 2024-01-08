#include "pch.h"

#include "Core/Application.h"
#include "Puppeteer/BaseLayer.h"

int main(int argc, char** argv) 
{
    Puppeteer::Application* app = new Puppeteer::Application({ "Puppeteer", 1280, 720 }, { argc, argv });
    app->PushLayer(new Puppeteer::BaseLayer());

    app->Run();
    delete app;
    return 0;
}
