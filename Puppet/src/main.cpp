#include "WS2tcpip.h"
#include "Network/PClientSocket.h"
#include "fstream"

static const std::string version = "V1.0.0";
static int port = 54000;

namespace Puppeteer
{
    static char* GetLocalIp() {
        WSADATA wsaData;                                                             
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed.\n";                                       
            return "";
        }

        int sock = socket(PF_INET, SOCK_DGRAM, 0);
        sockaddr_in loopback;

        if (sock == -1) {
            std::cerr << "Could not create socket\n";
            return "";
        }

        std::memset(&loopback, 0, sizeof(loopback));
        loopback.sin_family = AF_INET;
        loopback.sin_addr.s_addr = 1337;   // can be any IP address
        loopback.sin_port = htons(9);      // using debug port

        if (connect(sock, reinterpret_cast<sockaddr*>(&loopback), sizeof(loopback)) == -1) {
            closesocket(sock);
            std::cerr << "Could not connect\n";
            return "";
        }

        socklen_t addrlen = sizeof(loopback);
        if (getsockname(sock, reinterpret_cast<sockaddr*>(&loopback), &addrlen) == -1) {
            closesocket(sock);
            std::cerr << "Could not getsockname\n";
            return "";
        }

        closesocket(sock);
        
        char* buf = new char[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &loopback.sin_addr, buf, INET_ADDRSTRLEN) == 0x0) {
            std::cerr << "Could not inet_ntop\n";
            return "";
        }
        else {
            return buf;
            
        }      
        return "";
	}

	static int RunPuppet() {
        std::fstream logFile{"log.txt", std::ios::app};
        logFile << "Puppeteer started" << std::endl;
        //char* ip = GetLocalIp();
        // if (ip == "") {
		//	std::cerr << "Could not get local ip\n";
		//	return 1;
		//}
        //std::cout << "Local IP: " << std::string(ip) << std::endl;
        Networking::TCPServer client(Networking::IPV4, port, "0.0.0.0", 1);
        logFile << "Socket Created" << std::endl;
        logFile.close();
        //delete[] ip;
        StartPuppetSocket(&client);
		return 0;
	}
}


int main(int argc, char* argv[]) {
    if (argc == 2) {
        if(strcmp("-v", argv[1]) == 0) {
            std::cout << version;
            return 0;
        }
    }
	return Puppeteer::RunPuppet();
}



