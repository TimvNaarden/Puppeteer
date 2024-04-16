#include "WS2tcpip.h"
#include "Network/PClientSocket.h"
#include "fstream"
#include <thread>
#include <Windows.h>

#include <iostream>

static const std::string version = "V1.0.0";
static int port = 54000;


namespace Puppeteer {
	static int RunPuppet() {
        logFile << "Puppeteer started" << std::endl;
        logFile << "Version: " << version << std::endl;
        logFile << "Port: " << port << std::endl;

        Networking::TCPServer client(Networking::IPV4, port, "0.0.0.0", 1);
        if (!client.m_Connected) {
            logFile << "Failed to create socket" << std::endl;
            logFile << std::endl;   
            logFile.close();
        }
        logFile << "Socket created" << std::endl;
        StartPuppetSocket(&client);

        logFile << std::endl;
        logFile.close();
		return 0;
	}
} 


static int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if(std::string(GetCommandLineA()).find("-v") != std::string::npos) {
        std::cout << version;
        return 0;
    }
	return Puppeteer::RunPuppet();
}



