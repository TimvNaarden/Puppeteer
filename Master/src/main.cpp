#include "pch.h"

#include "Puppeteer/ConfigLayer.h"
#include "Puppeteer/InfoLayer.h"
#include "Puppeteer/GridLayer.h"
#include <Shlobj.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    WCHAR szPath[MAX_PATH];
    if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath)))  return 1;
    
    if (_wchdir(szPath) != 0) return 1;
    if (CreateDirectoryA("Puppeteer Master", NULL) == 0) {
		if(GetLastError() != ERROR_ALREADY_EXISTS) return 1;
	}
    if (_wchdir(L"Puppeteer Master") != 0) return 1;
    Puppeteer::app->PushLayer(new Puppeteer::InfoLayer());
    Puppeteer::app->PushLayer(new Puppeteer::GridLayer());
    Puppeteer::app->PushLayer(new Puppeteer::ConfigLayer());
    Puppeteer::app->Run();

    delete Puppeteer::app;
    return 0;
}