#pragma once

#include <cstdint>
#ifdef PLATFORM_WINDOWS
#define socklen_t int
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define RECVFROM_BUFFER char*
#define STARTWSA()                                                             \
  WSADATA wsaData;                                                             \
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {                             \
    std::cerr << "WSAStartup failed.\n";                                       \
    return;                                                                    \
  }

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif // PLATFORM_WINDOWS

#ifdef PLATFORM_LINUX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define RECVFROM_BUFFER void*
#define WSACleanup()
#define STARTWSA()
#define UINT16 uint16_t
#define SOCKET int
#define SOCKADDR sockaddr
#define closesocket(x) close(x)
#endif // PLATFORM_LINUX

#include "openssl/err.h"
#include "openssl/ssl.h"
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <string>
#pragma comment(lib, "libssl")
#pragma comment(lib, "libcrypto")

namespace Networking {
	// Globals to keep track of the amount of clients
	// So we know when to stop WSA(Windows) 
	extern int g_UDPServerCount;

	#ifndef PROT
	#define PROT
	enum iProtocol { IPV4 = AF_INET, IPV6 = AF_INET6 };
	#endif
	class UDPServer
	{
	public:
		/**
		 * @brief Construct a new TCPClient object
		 *
		 * @param iProt Desired protocol to use (IPV4 or IPV6)
		 * @param port Port to connect to
		 * @param ip IP to connect to
	    */
		UDPServer(iProtocol iProt, UINT16 port, char* ip);
		~UDPServer();
		/**
		 * @brief Send data to the server
		 *
	     * @param data Data to send
		 * @param ip IP to send to
         * @param port Port to send to
		 * @param length Length of the data to send
	     * @return int 0 on success, 1 on failure
 	   */
		int Send(const char* data, char* ip, UINT16 port, int length = 0);
		/**
	     * @brief Receive data from the server
		 *
		 * @param data Buffer to store the data
		 * @param ServerAddress Address of the server
		 * @return int 0 on success, 1 on failure
	    */
		int Receive(char*& data, sockaddr_in* ServerAddress);
	private:
		iProtocol m_iProt;
		SOCKET m_Socket;
	};
}

