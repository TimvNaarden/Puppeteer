#include "TCPClient.h"


namespace Networking {
	int g_TCPClientCount = 0;
	int g_TCPSSLClientCount = 0;

	TCPClient::TCPClient(iProtocol iProt, UINT16 port, char* ip, int SSL) {
		// Intialize Winsock (Windows only)
		if (g_TCPClientCount == 0) { STARTWSA(); }

		// Initialize SSL (If needed)
		if (SSL && g_TCPSSLClientCount == 0) {
			SSL_library_init();
			OpenSSL_add_all_algorithms();
			SSL_load_error_strings();
		}

		// Create a socket
		m_Socket = socket(iProt, SOCK_STREAM, IPPROTO_TCP);
		if (m_Socket == 0) {
			//throw std::runtime_error("Failed to create socket");
			return;
		}

		// Connect to the server
		sockaddr_in ServerAddress{};
		ServerAddress.sin_family = iProt;
		ServerAddress.sin_port = htons(port);
		ServerAddress.sin_addr.s_addr = inet_addr(ip);

		if (connect(m_Socket, (sockaddr*)&ServerAddress, sizeof(ServerAddress)) != 0) {
			//throw std::runtime_error("Failed to connect to server");
			closesocket(m_Socket);
			return;
		}

		// Set up SSL(If needed)
		if (SSL) {
			SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
			if (SSL_CTX_use_certificate_file(ctx, "Keys/client.crt", SSL_FILETYPE_PEM) <= 0) {
				//throw std::runtime_error("Could not load cert");
				return;
			}

			if (SSL_CTX_use_PrivateKey_file(ctx, "Keys/client.key", SSL_FILETYPE_PEM) <= 0) {
				//throw std::runtime_error("Could not load key");
				return;
			}
			m_SSL = SSL_new(ctx);
			SSL_set_fd(m_SSL, m_Socket);
			if (SSL_connect(m_SSL) != 1) {
				//throw std::runtime_error("Failed to connect to server with SSL");
				SSL_free(m_SSL);
				SSL_CTX_free(ctx);
				closesocket(m_Socket);
				return;
			}
			g_TCPSSLClientCount++;
		}
		g_TCPClientCount++;
	}

	TCPClient::~TCPClient() {
		// Clean up SSL (If needed)
		if (m_SSL == nullptr) {
			SSL_shutdown(m_SSL);
			SSL_free(m_SSL);
			g_TCPSSLClientCount--;

			if (g_TCPClientCount == 0) {
				SSL_CTX_free(SSL_get_SSL_CTX(m_SSL));
				ERR_free_strings();
				EVP_cleanup();
				CRYPTO_cleanup_all_ex_data();
			}
		}

		g_TCPClientCount--;

		// Clean up Winsock (Windows only)
		if (g_TCPClientCount == 0)
			WSACleanup();

		closesocket(m_Socket);
	}

	int TCPClient::Send(char*& data, int length) {
		size_t size = (length) ? length : strlen(data);
		std::string SizePacket = std::to_string(size);

		if (m_SSL != nullptr) {
			int ResultSize = SSL_write(m_SSL, SizePacket.c_str(), SizePacket.size());
			if (ResultSize <= 0) {
				//throw std::runtime_error("Failed to send size to server");
				return -1;
			}
			int TotalSent = 0;
			while (TotalSent < size) {
				int ResultData = SSL_write(m_SSL, data + TotalSent, size - TotalSent);
				if (ResultData <= 0) {
					//throw std::runtime_error("Failed to send data to server");
					return -1;
				}
				TotalSent += ResultData;
			}
		}
		else {
			int ResultSize = send(m_Socket, SizePacket.c_str(), SizePacket.size(), 0);
			if (ResultSize == -1) {
				//throw std::runtime_error("Failed to send size to server");
				return -1;
			}
			int TotalSent = 0;
			while (TotalSent < size) {
				int ResultData = send(m_Socket, data + TotalSent, size - TotalSent, 0);
				if (ResultData == -1) {
					//throw std::runtime_error("Failed to send data to server");
					return -1;
				}
				TotalSent += ResultData;
			}
		}
		return 0;
	}

	int TCPClient::Receive(char*& data) {
		char SizePacket[20]; // 64-bit integer can be at most 20 characters long

		if (m_SSL) {
			int ResultSize = SSL_read(m_SSL, SizePacket, 19);
			if (ResultSize <= 0) {
				//throw std::runtime_error("Failed to receive size from server");
				return -1;
			}
			SizePacket[ResultSize] = '\0';

			data = new char[std::stoi(SizePacket) + 1];
			int TotalReceived = 0;
			while (TotalReceived < std::stoi(SizePacket)) {
				int ResultData = SSL_read(m_SSL, (RECVFORM_BUFFER)data + TotalReceived, std::stoi(SizePacket) - TotalReceived);
				if (ResultData <= 0) {
					//throw std::runtime_error("Failed to receive data from server");
					return -1;
				}
				TotalReceived += ResultData;
			}
			data[TotalReceived] = '\0';
		}
		else {
			int ResultSize = recv(m_Socket, SizePacket, 19, 0);
			if (ResultSize == -1) {
				//throw std::runtime_error("Failed to receive size from server");
				return -1;
			}
			SizePacket[ResultSize] = '\0';
			data = new char[std::stoi(SizePacket) + 1];
			int TotalReceived = 0;
			while (TotalReceived < std::stoi(SizePacket)) {
				int ResultData = recv(m_Socket, (RECVFORM_BUFFER)data + TotalReceived, std::stoi(SizePacket) - TotalReceived, 0);
				if (ResultData == -1) {
					//throw std::runtime_error("Failed to receive data from server");
					return -1;
				}
				TotalReceived += ResultData;
			}
			data[TotalReceived] = '\0';
		}

		return 0;
	}
} // namespace Networking
