#pragma once
#ifdef PLATFORM_WINDOWS
#define socklen_t int
#define _WINSOCK_DEPRECATED_NO_WARNINGS

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

#define WSACleanup()
#define STARTWSA()
#define UINT16 uint16_t
#define SOCKET int
#define SOCKADDR sockaddr
#define closesocket(x) close(x)
#endif // PLATFORM_LINUX

#include <cstring>
#include <iostream>
#include <string>

#include "openssl/err.h"
#include "openssl/ssl.h"

#pragma comment(lib, "libssl")
#pragma comment(lib, "libcrypto")

namespace Networking {
	// Globals to keep track of the amount of clients
	// So we know when to stop WSA(Windows) or shutdown ssl
	extern int g_TCPServerCount;
	extern int g_TCPSSLServerCount;

	#ifndef PROT
	#define PROT
	enum iProtocol { IPV4 = AF_INET, IPV6 = AF_INET6 };
	#endif
	
	class TCPServer {
	public:
		/**
		 * @brief The constructor for the TCPServer class
		 *
		 * @param iProt The internet protocol to use (IPV4 or IPV6)
		 * @param port The port to listen on
		 * @param ip The ip to listen on
		 * @param SSL Enable SSL (default 0)
		 */
		TCPServer(iProtocol iProt, UINT16 port, char* ip, int SSL = 0);
		~TCPServer();

		/**
		 * @brief Send data to a client
		 *
		 * @param client The client to send the data to
		 * @param data The data to send
		 * @param length The length of the data to send (default 0)
		 * @param c_SSL The SSL context to use (default nullptr)
		 *
		 * @return int 0 if successful, -1 if failed
		 */
		int Send(SOCKET client, char*& data, int length = 0, SSL* c_SSL = nullptr) const;

		/**
		 * @brief Receive data from a client
		 *
		 * @param client The client to receive the data from
		 * @param data The buffer to store the data in
		 * @param c_SSL The SSL context to use (default nullptr)
		 *
		 * @return int 0 if successful, -1 if failed
		 */
		int Receive(SOCKET client, char*& data, SSL* c_SSL = nullptr);

		/**
		 * @brief Start listening for clients
		 *
		 * This is a loop be aware of that
		 * @param callback pointer to the function that will be called for each client
		 */
		void StartListening(void (*callback)(TCPServer*, SOCKET, SSL*));

	private:
		SOCKET m_Socket = 0;

		int m_SSL = 0;
	};

} // namespace Networking