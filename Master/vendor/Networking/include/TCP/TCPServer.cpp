#include "TCPServer.h"

namespace Networking {
	int g_TCPServerCount = 0;
	int g_TCPSSLServerCount = 0;

	TCPServer::TCPServer(iProtocol iProt, UINT16 port, char* ip, int SSL) {
		// Intialize Winsock (Windows only)
		if (g_TCPServerCount == 0) {
			STARTWSA();
		}

		m_SSL = (SSL) ? 1 : 0;
		// Initialize SSL (If needed)
		if (SSL && g_TCPSSLServerCount == 0) {
			m_SSL = 1;
			SSL_library_init();
			OpenSSL_add_all_algorithms();
			SSL_load_error_strings();
			g_TCPSSLServerCount++;
		}

		// Create a socket
		m_Socket = socket(iProt, SOCK_STREAM, IPPROTO_TCP);
		if (m_Socket == 0) {
			throw std::runtime_error("Failed to create socket");
			return;
		}

		sockaddr_in ServerAddress{};
		ServerAddress.sin_family = iProt;
		ServerAddress.sin_port = htons(port);
		ServerAddress.sin_addr.s_addr = inet_addr(ip);

		// Bind the socket to the IP and port
		if (bind(m_Socket, (sockaddr*)&ServerAddress, sizeof(ServerAddress)) == -1) {
			throw std::runtime_error("Failed to bind socket");
			return;
		}

		// Start Listening
		if (listen(m_Socket, 1) == -1) {
			throw std::runtime_error("Failed to listen on socket");
			return;
		}
		g_TCPServerCount++;
	}

	TCPServer::~TCPServer() {
		if (m_SSL)
			g_TCPSSLServerCount--;
		// Clean up SSL (If needed)
		if (m_SSL && g_TCPSSLServerCount == 0) {
			ERR_free_strings();
			EVP_cleanup();
			CRYPTO_cleanup_all_ex_data();
		}
		g_TCPServerCount--;

		// Clean up Winsock (Windows only)
		if (g_TCPServerCount == 0)
			WSACleanup();

		closesocket(m_Socket);
	}

	int TCPServer::Send(SOCKET client, char*& data, int length, SSL* c_SSL) const {
		size_t size = (length) ? length : strlen(data);
		std::string SizePacket = std::to_string(size);

		if (m_SSL) {
			int ResultSize = SSL_write(c_SSL, SizePacket.c_str(), SizePacket.size());
			if (ResultSize <= 0) {
				throw std::runtime_error("Failed to send size to server");
				return -1;
			}
			int TotalSent = 0;
			while (TotalSent < size) {
				int ResultData = SSL_write(c_SSL, data + TotalSent, size - TotalSent);
				if (ResultData <= 0) {
					throw std::runtime_error("Failed to send data to server");
					return -1;
				}
				TotalSent += ResultData;
			}
		}
		else {
			int ResultSize = send(client, SizePacket.c_str(), SizePacket.size(), 0);
			if (ResultSize == -1) {
				throw std::runtime_error("Failed to send size to server");
				return -1;
			}
			int TotalSent = 0;
			while (TotalSent < size) {
				int ResultData = send(client, data + TotalSent, size - TotalSent, 0);
				if (ResultData == -1) {
					throw std::runtime_error("Failed to send data to server");
					return -1;
				}
				TotalSent += ResultData;
			}
		}
		return 0;
	}

	int TCPServer::Receive(SOCKET client, char*& data, SSL* c_SSL) {
		char SizePacket[20]; // 64-bit integer can be at most 20 characters long
		if (c_SSL) {
			int ResultSize = SSL_read(c_SSL, SizePacket, 19);
			if (ResultSize <= 0) {
				throw std::runtime_error("Failed to receive size from client");
				return -1;
			}
			SizePacket[ResultSize] = '\0';

			data = new char[std::stoi(SizePacket) + 1];
			int TotalReceived = 0;
			while (TotalReceived < std::stoi(SizePacket)) {
				int ResultData = SSL_read(c_SSL, data + TotalReceived, std::stoi(SizePacket));
				if (ResultData <= 0) {
					throw std::runtime_error("Failed to receive data from client");
					return -1;
				}
				TotalReceived += ResultData;
			}
			data[TotalReceived] = '\0';
		}
		else {
			int ResultSize = recv(client, SizePacket, 19, 0);
			if (ResultSize == -1) {
				throw std::runtime_error("1Failed to receive size from client");
				return -1;
			}
			SizePacket[ResultSize] = '\0';
			data = new char[std::stoi(SizePacket) + 1];
			int TotalReceived = 0;
			while (TotalReceived < std::stoi(SizePacket)) {
				int ResultData = recv(client, data + TotalReceived, std::stoi(SizePacket), 0);
				if (ResultData == -1) {
					throw std::runtime_error("Failed to receive data from client");
					return -1;
				}
				TotalReceived += ResultData;
			}
			data[TotalReceived] = '\0';
		}

		return 0;
	}

	void TCPServer::StartListening(void (*callback)(TCPServer*, SOCKET, SSL*)) {
		while (true) {
			sockaddr_in ClientAddress;
			socklen_t ClientAddressSize = sizeof(ClientAddress);
			SOCKET ClientSocket =
				accept(m_Socket, (sockaddr*)&ClientAddress, &ClientAddressSize);
			if (ClientSocket <= 0) {
				throw std::runtime_error("Failed to accept client");
				continue;
			}
			if (m_SSL) {
				SSL_CTX* ctx = SSL_CTX_new(SSLv23_server_method());
				if (SSL_CTX_use_certificate_file(ctx, "Keys/server.crt",
					SSL_FILETYPE_PEM) <= 0) {
					throw std::runtime_error("Could not load cert");
					return;
				}

				if (SSL_CTX_use_PrivateKey_file(ctx, "Keys/server.key",
					SSL_FILETYPE_PEM) <= 0) {
					throw std::runtime_error("Could not load key");
					return;
				}
				SSL* ssl = SSL_new(ctx);
				SSL_set_fd(ssl, ClientSocket);
				if (SSL_accept(ssl) != 1) {
					throw std::runtime_error("Failed to accept SSL connection");
					continue;
				}
				callback(this, ClientSocket, ssl);
			}
			else {
				callback(this, ClientSocket, NULL);
			}
		}
	}
} // namespace Networking
