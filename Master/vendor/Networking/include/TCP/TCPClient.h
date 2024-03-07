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
	// So we know when to stop WSA(Windows) or shutdown ssl
	extern int g_TCPClientCount;
	extern int g_TCPSSLClientCount;

	#ifndef PROT
	#define PROT
	enum iProtocol { IPV4 = AF_INET, IPV6 = AF_INET6 };
	#endif

	class TCPClient {
	public:
		/**
		 * @brief Construct a new TCPClient object
		 *
		 * @param iProt Desired protocol to use (IPV4 or IPV6)
		 * @param port Port to connect to
		 * @param ip IP to connect to
		 * @param SSL Enable SSL (default 0)
		 */
		TCPClient(iProtocol iProt, UINT16 port, char* ip, int SSL = 0);
		~TCPClient();

		/**
		 * @brief send data to the server
		 *
		 * @param data Data to send
		 * @param length Length of the data (default 0)
		 * @return int 0 if successful, -1 if failed
		 */
		int Send(char*& data, int length = 0);

		/**
		 * @brief Receive data from the server
		 * Delete the buffer after use
		 * @param data Buffer to store the data
		 * @return int 0 if successful, -1 if failed
		 */
		int Receive(char*& data);

	private:
		SOCKET m_Socket = 0;
		SSL* m_SSL = nullptr;
	};

} // namespace Networking
