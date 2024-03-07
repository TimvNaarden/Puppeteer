#include "pch.h"

#include "Puppeteer/PuppetLayer.h"
#include "Puppeteer/ConfigLayer.h"
#include "Puppeteer/InfoLayer.h"

int main(int argc, char** argv)  {

    Puppeteer::app->PushLayer(new Puppeteer::InfoLayer());
    Puppeteer::app->PushLayer(new Puppeteer::ConfigLayer());

    //Puppeteer::app->PushLayer(new Puppeteer::PuppetLayer("192.168.2.58", WriteJson(Credentials)));
    //Puppeteer::app->PushLayer(new Puppeteer::PuppetLayer("192.168.2.62", WriteJson(Credentials)));
    Puppeteer::app->Run();
    delete Puppeteer::app;
    return 0;
}
