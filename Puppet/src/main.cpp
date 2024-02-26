#include "WS2tcpip.h"
#include "Network/PClientSocket.h"




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
        
        char buf[INET_ADDRSTRLEN];
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
        char* ip = GetLocalIp();
        if (ip == "") {
			std::cerr << "Could not get local ip\n";
			return 1;
		}
		Puppeteer::PClientSocket client;
		client.Create(IPV4, TCP, SERVER, 54000, ip, true);
		client.HandleServerSocket();
		return 0;
	}
}


int main() {
	return Puppeteer::RunPuppet();
}
