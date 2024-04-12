#include "pch.h"

#include "Puppeteer/ConfigLayer.h"
#include "Puppeteer/InfoLayer.h"
#include "Puppeteer/GridLayer.h"

int main(int argc, char** argv)  {
    Puppeteer::app->PushLayer(new Puppeteer::InfoLayer());
    Puppeteer::app->PushLayer(new Puppeteer::GridLayer());
    Puppeteer::app->PushLayer(new Puppeteer::ConfigLayer());
    Puppeteer::app->Run();

    delete Puppeteer::app;
    return 0;
}