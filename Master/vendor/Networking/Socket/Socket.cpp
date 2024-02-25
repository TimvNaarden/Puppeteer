#include "Socket.h"
int SSLCounter = 0;
int SocketCounter = 0;

Socket::Socket() {
    if (SocketCounter == 0) {
        STARTWSA();
    }
      SocketCounter++;
      // Init vars
      m_StopListen = 0;
      m_Socket = INVALID_SOCKET;
      m_Type = TCP;
      m_sslctx = nullptr;
      m_ssl = nullptr;
      m_CommunicationType = SERVER;
      m_SSLEnabled = 0;
  
}

Socket::~Socket() {
  SocketCounter--;
  if (SocketCounter == 0) WSACleanup();
  m_StopListen = 1;

  if (m_SSLEnabled) {
    CleanupSSL();
  }

  closesocket(m_Socket);
}



void Socket::HandleServerSocket() {
  if (m_Type == UDP) {
    while (true) {
      Listen(m_Socket, &m_ssl);
    }
  }

  while (!m_StopListen) {
    sockaddr_in addr{};
    socklen_t addrlen = sizeof(addr);
    SOCKET clientSocket = accept(m_Socket, (struct sockaddr *)&addr, &addrlen);

    if (clientSocket == INVALID_SOCKET) {
      continue;
    }
    SSL* pssl = nullptr;
    if (m_SSLEnabled) {
      BIO *sbio = BIO_new_socket(clientSocket, BIO_NOCLOSE);
      EncryptSocket(&pssl);
      SSL_set_bio(pssl, sbio, sbio);

      int ret = SSL_accept(pssl);
      if (ret <= 0) {
        SERVERCMD("Failed to accept SSL");
        SERVERCMD(SSL_get_error(pssl, ret));
        continue;
      }
    }
    std::thread t1 = std::thread(&Socket::HandleClient, this, clientSocket, &pssl);
    t1.detach();
  }
}

void Socket::HandleClient(SOCKET clientSocket, SSL** pssl) {
    if (!AcceptConnection()) {
        closesocket(clientSocket);
        if(m_SSLEnabled) {
            SSL_shutdown(*pssl);
            SSL_free(*pssl);
        }
        return;
    }

    while (true) {
        if (Listen(clientSocket, pssl)) {
            break;
        }
    }
    closesocket(clientSocket);
    if (m_SSLEnabled) {
        SSL_shutdown(*pssl);
        SSL_free(*pssl);
    }
}

int Socket::Listen(SOCKET clientSocket, SSL** pssl) {
  // Default implementation prints the received packet
  char *result = ReceivePacket(clientSocket, pssl);
  if (result == nullptr) {
    return 0;
  } else if (result == "Con Closed") {
    SERVERCMD("Connection closed");
    return 1;
  }

  SERVERCMD(result);

  free(result);

  // return 1 to break the loop
  return 0;
}

int Socket::AcceptConnection() {
  // Logic to accept connection
  return 1;
}

int Socket::Create(internetProtocol iprotocol, socketType type, communicationType ctype, int port, char *ip,int SSLEncryption) {
  int protocol = (type == UDP) ? IPPROTO_UDP : IPPROTO_TCP;
  m_SSLEnabled = SSLEncryption;
  m_CommunicationType = ctype;
  m_Type = type;
  m_Socket = socket(iprotocol, type, protocol);

  if (m_Socket == INVALID_SOCKET) {
    std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
    WSACleanup();
    return 1;
  }

  sockaddr_in service{};
  service.sin_family = iprotocol;
  service.sin_addr.s_addr = inet_addr(ip);
  service.sin_port = htons(port);

  if (type == UDP && ctype == CLIENT) return 0;

  if (ctype == SERVER) {
    if (bind(m_Socket, (SOCKADDR *)&service, sizeof(service)) == SOCKET_ERROR) {
      SERVERCMD("bind() failed.");
      closesocket(m_Socket);
      return 1;
    }

    if (type == UDP) return 0;

    if (listen(m_Socket, 1) == SOCKET_ERROR) {
      SERVERCMD("listen(): Error listening on socket ");
      closesocket(m_Socket);
      return 1;
    }
    return 0;
  }

  if (ctype == CLIENT) {
    if (connect(m_Socket, (SOCKADDR *)&service, sizeof(service)) == SOCKET_ERROR) {
      CLIENTCMD("Failed to connect");
      closesocket(m_Socket);
      return 1;
    }

    if (SSLEncryption) {
      EncryptSocket(&m_ssl);
      BIO *sbio = BIO_new_socket(m_Socket, BIO_NOCLOSE);
      SSL_set_bio(m_ssl, sbio, sbio);
      int ret = SSL_connect(m_ssl);
      if (ret <= 0) {
        CLIENTCMD(SSL_get_error(m_ssl, ret));
        return 1;
      }
    }
  }
  if (SSLEncryption) SSLCounter++;
  return 0;
}

int Socket::SendPacket(char *packet, SSL** pssl, size_t size, SOCKET dest, SOCKADDR *destaddr) {

   if (m_Type == UDP) {
    if (!destaddr) {
      std::cerr << "No destination address" << std::endl;
      return 1;
    }
    return SendUDP(packet, destaddr, size);
  }
  if(!pssl) pssl = &m_ssl;
  if (!dest) dest = m_Socket;
  if (m_SSLEnabled) return SendSSL(packet, pssl, dest, size);
  else return Send(packet, dest, size);
}

char *Socket::ReceivePacket(SOCKET source, SSL** pssl) {
  if (!pssl) pssl = &m_ssl;
  if (!source) source = m_Socket;
  if (m_Type == UDP) return ReceiveUDP(source);
  if (m_SSLEnabled) return ReceiveSSL(source, pssl);
  else return Receive(source);
}

// Send and receive functions
int Socket::Send(char *packet, SOCKET dest, size_t sizeinput) {
    size_t size;
    if (sizeinput != 0) {
	    size = sizeinput;
    }
    else {
	    size = strlen(packet);
    }
    std::string stringSize = std::to_string(size + 1);

    int result;

    result = send(dest, stringSize.c_str(), 64, 0);
    if (result == SOCKET_ERROR) {
    if (result == -1) {
        std::cerr << "Connection closed." << std::endl;
        return -1;
    }
    std::cerr << "send size failed: " << WSAGetLastError() << std::endl;
    return 1;
    }
    //Make sure all data is sent
    for (int i = 0; i < size + 1; i += result) {
    result = send(dest, packet + i, size + 1 - i, 0);
    if (result == SOCKET_ERROR) {
        if (result == -1) {
	    std::cerr << "Connection closed." << std::endl;
	    return -1;
	    }
	    std::cerr << "send data failed: " << WSAGetLastError() << std::endl;
	    return 1;
    }
    }

    return 0;
}

int Socket::SendSSL(char *packet, SSL** pssl, SOCKET dest, size_t sizeinput) {
    size_t size;
    if (sizeinput != 0) {
        size = sizeinput;
    }
    else {
        size = strlen(packet);
    }
    std::string stringSize = std::to_string(size + 1);

    int result;

   result = SSL_write(*pssl, stringSize.c_str(), 64);
    if (result == SOCKET_ERROR) {
        if (result == -1) {
            std::cerr << "Connection closed." << std::endl;
            return -1;
        }
        std::cerr << "send size failed: " << WSAGetLastError() << std::endl;
        return 1;
    }
    //Make sure all data is sent
    for (int i = 0; i < size + 1; i += result) {
        result = SSL_write(*pssl, packet + i, size + 1);
        if (result == SOCKET_ERROR) {
            if (result == -1) {
                std::cerr << "Connection closed." << std::endl;
                return -1;
            }
            std::cerr << "send data failed: " << WSAGetLastError() << std::endl;
            return 1;
        }
    }

    return 0;
}

int Socket::SendUDP(char *packet, SOCKADDR *destaddr, size_t sizeinput) {
    size_t size;
    if (sizeinput != 0) {
        size = sizeinput;
    }
    else {
        size = strlen(packet);
    }
    std::string stringSize = std::to_string(size + 1);
    int result;

    result = sendto(m_Socket, stringSize.c_str(), 64, 0, destaddr,
                    sizeof(sockaddr_in));
    if (result == SOCKET_ERROR) {
    if (result == -1) {
        std::cerr << "Connection closed." << std::endl;
        return -1;
    }
    std::cerr << "send size failed: " << WSAGetLastError() << std::endl;
    return 1;
    }
    //Make sure all data is sent
    for (int i = 0; i < size + 1; i += result) {
        result = sendto(m_Socket, packet + i, size + 1, 0, destaddr, sizeof(sockaddr_in));
        if (result == SOCKET_ERROR) {
            if (result == -1) {
                std::cerr << "Connection closed." << std::endl;
                return -1;
            }
            std::cerr << "send data failed: " << WSAGetLastError() << std::endl;
            return 1;
        }
    }

    return 0;
}

char *Socket::Receive(SOCKET source) {
  size_t size = 65536;
  char *stringSize = (char *)malloc(64);
  int result;

  if (stringSize == nullptr) {
    std::cerr << "Failed to allocate memory" << std::endl;
    return nullptr;
  }

  result = recv(source, (char *)stringSize, 64, 0);
  if (result == SOCKET_ERROR) {
    if (result == -1) {
      std::cerr << "Connection closed." << std::endl;
      return "Con Closed";
    }
    std::cerr << "recv size failed: " << WSAGetLastError() << std::endl;
    return nullptr;
  }
  size = atoi(stringSize);
  char *packet = new char[size];

  //Make sure all data is received
  for (int i = 0; i < size; i += result) {
	result = recv(source, packet + i, size - i, 0);
    if (result == SOCKET_ERROR) {
        if (result == -1) {
		std::cerr << "Connection closed." << std::endl;
		return "Con Closed";
	  }
	  std::cerr << "recv data failed: " << WSAGetLastError() << std::endl;
	  return nullptr;
	}
  }

  // Free memory
  free(stringSize);

  if (packet[0] == -3) {
    std::cerr << "Connection closed." << std::endl;
    return "Con Closed";
  }
  return packet;
}

char *Socket::ReceiveSSL(SOCKET source, SSL** pssl) {
    size_t size = 65536;
    char *stringSize = (char *)malloc(64);
    int result;
    result = SSL_read(*pssl, stringSize, 64);
    if (result == SOCKET_ERROR) {
        if (result == -1) {
            std::cerr << "Connection closed." << std::endl;
            return "Con Closed";
        }
        std::cerr << "recv size failed: " << WSAGetLastError() << std::endl;
        return nullptr;
    }
    if (stringSize == nullptr) {
        std::cerr << "Could not get stringsize" << std::endl;
        return "Con Closed";
    }
    size = atoi(stringSize);
    char *packet = (char *)malloc(size);

    //Make sure all data is received
    for (int i = 0; i < size; i += result) {
        result = SSL_read(*pssl, packet + i, size);
        if (result == SOCKET_ERROR) {
            if (result == -1) {
                std::cerr << "Connection closed." << std::endl;
                return "Con Closed";
            }
            std::cerr << "recv data failed: " << WSAGetLastError() << std::endl;
            return nullptr;
        }
    }
    // Free memory
    free(stringSize);

    if (packet[0] == -3) {
        std::cerr << "Connection closed." << std::endl;
        return "Con Closed";
    }
    return packet;
}

char *Socket::ReceiveUDP(SOCKET source) {
  size_t size = 65536;
  char *stringSize = (char *)malloc(64);
  int result;

  sockaddr_in client{};
  socklen_t clientSize = sizeof(client);
  if (stringSize == nullptr) {
    std::cerr << "Failed to allocate memory" << std::endl;
    return nullptr;
  }

  result = recvfrom(source, (RECVFROM_BUFFER *)stringSize, 64, 0,
                    (sockaddr *)&client, &clientSize);
  if (result == SOCKET_ERROR) {
    if (result == -1) {
      std::cerr << "Connection closed." << std::endl;
      return "Con Closed";
    }
    std::cerr << "recv size failed: " << WSAGetLastError() << std::endl;
    return nullptr;
  }
  size = atoi(stringSize);
  char *packet = new char[size];

  //Make sure all data is received
  for (int i = 0; i < size; i += result) {
      result = recvfrom(source, packet + i, size, 0, (sockaddr*)&client, &clientSize);
      if (result == SOCKET_ERROR) {
          if (result == -1) {
              std::cerr << "Connection closed." << std::endl;
              return "Con Closed";
          }
          std::cerr << "recv data failed: " << WSAGetLastError() << std::endl;
          return nullptr;
      }
  }
  // Free memory
  free(stringSize);

  if (packet[0] == -3) {
    std::cerr << "Connection closed." << std::endl;
    return "Con Closed";
  }
  return packet;
}

// SSL functions
void Socket::EncryptSocket(SSL** pssl) {
    if (m_sslctx == nullptr) {
        if (CreateSSLContext()) {
            std::cerr << "Failed to create SSL Context" << std::endl;
	        return;
	    }
    }

    *pssl = SSL_new(m_sslctx);
    if (*pssl == nullptr) {
		std::cerr << "Failed to create SSL" << std::endl;
		return;
	}

    return;
}

int Socket::CreateSSLContext() {
    const SSL_METHOD *method;
    if (m_CommunicationType == CLIENT) method = TLS_client_method();
    else method = TLS_server_method();

    m_sslctx = SSL_CTX_new(method);
    if (!m_sslctx) {
        std::cerr << "Unable to create SSL context" << std::endl;
        return 1;
    }
    if (m_CommunicationType == CLIENT) {
        if (SSL_CTX_use_certificate_file(m_sslctx, "Keys/client.crt", SSL_FILETYPE_PEM) <= 0) {
            std::cerr << "Could not load cert" << std::endl;
            return 1;
        }

        if (SSL_CTX_use_PrivateKey_file(m_sslctx, "Keys/client.key", SSL_FILETYPE_PEM) <= 0) {
            std::cerr << "Could not load key" << std::endl;
            return 1;
        }
    } else if (m_CommunicationType == SERVER) {
        if (SSL_CTX_use_certificate_file(m_sslctx, "Keys/server.crt", SSL_FILETYPE_PEM) <= 0) {
            std::cerr << "Could not load cert" << std::endl;
            return 1;
        }

        if (SSL_CTX_use_PrivateKey_file(m_sslctx, "Keys/server.key", SSL_FILETYPE_PEM) <= 0) {
            std::cerr << "Could not load key" << std::endl;
            return 1;
        }
    } else {
        return 1;
    }
    return 0;
}

void Socket::CleanupSSL() {
    if (SSLCounter == 1) {
    SSL_shutdown(m_ssl);
    //SSL_free(m_ssl);
    SSL_CTX_free(m_sslctx);

    EVP_cleanup();
    ERR_free_strings();
    }
    SSLCounter--;
}
