#pragma once

#include <cstdint>
#ifdef PLATFORM_WINDOWS
#define socklen_t int
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define RECVFORM_BUFFER char*
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
#define RECVFORM_BUFFER void*
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
	extern int g_UDPClientCount;

	#ifndef PROT
	#define PROT
	enum iProtocol { IPV4 = AF_INET, IPV6 = AF_INET6 };
	#endif

	class UDPClient
	{
	public:
		/**
		 * @brief Construct a new TCPClient object
		 *
		 * @param iProt Desired protocol to use (IPV4 or IPV6)
		 * @param port Port to connect to
		 * @param ip IP to connect to
		 * @return int 0 if successful, -1 if failed
		*/
		UDPClient(iProtocol iProt);

		/**
		 * @brief Send a message to the server
		 *
		 * @param data Message to send
		 * @param ip IP to send to
		 * @param port Port to use
		 * @return int 0 if successful, -1 if failed
	    */
		int Send(const char* data, char* ip, UINT16 port, int length = 0);

		/**
		 * @brief Receive a message from the server
		 *
		 * @param data Buffer to store the message
         * @param ServerAddress Address of the server
		 * @return int 0 if successful, -1 if failed
		*/
		int Receive(char*& data, sockaddr_in* ServerAddress);
		/**
		 * @brief Destroy the TCPClient object
		*/
		~UDPClient();
	private:
		SOCKET m_Socket;
		iProtocol m_iProt;
	};
}

