#pragma once

#ifdef PLATFORM_WINDOWS
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define socklen_t int
#define RECVFROM_BUFFER char
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define AF_INET6 23

#define STARTWSA()                                                             \
  WSADATA wsaData;                                                             \
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {                             \
    std::cerr << "WSAStartup failed.\n";                                       \
    return;                                                                    \
  }                                                                                    
#pragma comment (lib, "crypt32")
#pragma comment(lib, "legacy_stdio_definitions")
#pragma comment(lib, "libssl")
#pragma comment(lib, "libcrypto")
 
#endif // PLATFORM_WINDOWS

#ifdef PLATFORM_LINUX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define WSACleanup()
#define WSAGetLastError() ""
#define WSADATA int
#define STARTWSA()
#define MAKEWORD()
#define SOCKET int
#define SOCKADDR sockaddr
#define closesocket(x) close(x)
#define INVALID_SOCKET 0
#define SOCKET_ERROR -1
#define RECVFROM_BUFFER void

#endif // PLATFORM_LINUX

extern int SSLCounter;

#include <future>
#include <iostream>

#include "Openssl/err.h"
#include "OpenSSL/ssl.h"


// Macro's
#define SERVERCMD(x) std::cout << "Server: " << x << std::endl;
#define CLIENTCMD(x) std::cout << "Client: " << x << std::endl;
enum socketType {
  TCP = SOCK_STREAM,
  UDP = SOCK_DGRAM,
};

enum communicationType { CLIENT = 0, SERVER = 1 };

enum internetProtocol { IPV4 = AF_INET, IPV6 = AF_INET6 };

class Socket {
public:
  Socket();
  ~Socket();

  // Creates a socket
  // Returns 0 if successful
  // After creating call the appropriate function to handle the socket
  virtual int Create(internetProtocol iprotocol, // IPV4 or IPV6
                     socketType type,            // TCP or UDP
                     communicationType ctype,    // Client or Server
                     int port = 0, // If using TCP or UDP server specify port
                     char *ip = "127.0.0.1",   // Specify ip for servers to bind
                                               // or tcp client to connect to
                     int SSLEncryption = false // Enable SSL encryption
  );

  // This function will wait for incoming connections
  // It will be called in a loop until m_stoplisten is set to 1
  // Default implementation will accept incoming clients and call the
  // HandleClient() function in a new thread
  virtual void HandleServerSocket();

  // Optional function to accept connection
  // Will return 1 if not altered
  // To refuse the connection return 0
  // TCP server only
  virtual int AcceptConnection();

  // Send a packet
  // Do not specify a destination if using TCP client or server
  // Returns -1 if connection is closed
  // Returns 0 if successful
  // Returns 1 if failed
  virtual int SendPacket(char *packet, size_t size = 0, SOCKET dest = 0,
                         SOCKADDR *destaddr = {});

  // Receive a packet
  // Delete the packet after
  // Returns "Con Closed " if connection is closed
  // Returns Nullptr if failed
  // Returns the packet if successful
  virtual char *ReceivePacket(SOCKET source = 0);

  // Determes what to do with the client socket after accepting connection
  // Calls AcceptConnection() function to check if connection should be
  // intialized Calls listen() function in a loop until it returns 1 to break
  // the loop
  virtual void HandleClient(SOCKET clientSocket);

  // After the server socket has an connection with a client socket
  // this function wil run in a loop until it returns 1
  // To break the loop return 1
  virtual int Listen(SOCKET clientSocket);


  int m_StopListen = 1;
  socketType m_Type;
  SOCKET m_Socket;
  communicationType m_CommunicationType;

  // These functions here, are not ment to be changed
  // 
  // Send and receive functions
  virtual int Send(char *packet, SOCKET dest, size_t sizeinput);
  virtual int SendUDP(char *packet, SOCKADDR *destaddr, size_t sizeinput);
  virtual int SendSSL(char *packet, SOCKET dest, size_t sizeinput);

  virtual char *Receive(SOCKET source);
  virtual char *ReceiveSSL(SOCKET source);
  virtual char *ReceiveUDP(SOCKET source);

  // SSL functions
  int EncryptSocket();
  int CreateSSLContext();
  void CleanupSSL();

  // SSL variables
  SSL_CTX *m_sslctx;
  SSL *m_ssl;
};
