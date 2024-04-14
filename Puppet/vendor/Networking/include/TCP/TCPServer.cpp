#include "TCPServer.h"
#include "fstream"
#include <thread>
static DWORD g_dwSessionID = WTSGetActiveConsoleSessionId();
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
			std::cerr << "Failed to create socket" << std::endl;
			return;
		}

		sockaddr_in ServerAddress{};
		ServerAddress.sin_family = iProt;
		ServerAddress.sin_port = htons(port);
		ServerAddress.sin_addr.s_addr = inet_addr(ip);

		// Bind the socket to the IP and port
		if (bind(m_Socket, (sockaddr*)&ServerAddress, sizeof(ServerAddress)) == -1) {
			std::cerr << "Failed to bind socket" << std::endl;
			return;
		}

		// Start Listening
		if (listen(m_Socket, 1) == -1) {
			std::cerr << "Failed to listen" << std::endl;
			return;
		}
		g_TCPServerCount++;
		m_Connected = 1;
	}

	TCPServer::~TCPServer() {
		if (m_SSL) g_TCPSSLServerCount--;
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
				std::cerr << "Failed to send size to server" << std::endl;
				return -1;
			}
			int TotalSent = 0;
			while (TotalSent < size) {
				int ResultData = SSL_write(c_SSL, data + TotalSent, size - TotalSent);
				if (ResultData <= 0) {
					std::cerr << "Failed to send data to server" << std::endl;
					return -1;
				}
				TotalSent += ResultData;
			}
		}
		else {
			int ResultSize = send(client, SizePacket.c_str(), SizePacket.size(), 0);
			if (ResultSize == -1) {
				std::cerr << "Failed to send size to server" << std::endl;
				return -1;
			}
			int TotalSent = 0;
			while (TotalSent < size) {
				int ResultData = send(client, data + TotalSent, size - TotalSent, 0);
				if (ResultData == -1) {
					std::cerr << "Failed to send data to server" << std::endl;
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
				std::cerr << "Failed to receive size from client" << std::endl;
				return -1;
			}
			SizePacket[ResultSize] = '\0';

			data = new char[std::stoi(SizePacket) + 1];
			int TotalReceived = 0;
			while (TotalReceived < std::stoi(SizePacket)) {
				int ResultData = SSL_read(c_SSL, data + TotalReceived, std::stoi(SizePacket));
				if (ResultData <= 0) {
					std::cerr << "Failed to receive data from client" << std::endl;
					return -1;
				}
				TotalReceived += ResultData;
			}
			data[TotalReceived] = '\0';
		}
		else {
			int ResultSize = recv(client, SizePacket, 19, 0);
			if (ResultSize == -1) {
				std::cerr << "Failed to receive size from client" << std::endl;
				return -1;
			}
			SizePacket[ResultSize] = '\0';
			data = new char[std::stoi(SizePacket) + 1];
			int TotalReceived = 0;
			while (TotalReceived < std::stoi(SizePacket)) {
				int ResultData = recv(client, data + TotalReceived, std::stoi(SizePacket), 0);
				if (ResultData == -1) {
					std::cerr << "Failed to receive data from client" << std::endl;
					return -1;
				}
				TotalReceived += ResultData;
			}
			data[TotalReceived] = '\0';
		}

		return 0;
	}
	static bool Encrypt(SSL_CTX* ctx) {
		EVP_PKEY* pkey;
		pkey = EVP_PKEY_new();

		RSA* rsa;
		rsa = RSA_generate_key(2048, RSA_F4, nullptr, nullptr);

		EVP_PKEY_assign_RSA(pkey, rsa);

		// Generate X509 certificate
		X509* x509;
		x509 = X509_new();
		ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
		X509_gmtime_adj(X509_get_notBefore(x509), 0);
		X509_gmtime_adj(X509_get_notAfter(x509), 31536000L);
		X509_set_pubkey(x509, pkey);

		X509_NAME* name;
		name = X509_get_subject_name(x509);
		X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char*)"NL", -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char*)"CPP_Networking", -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)"www.github.com/timvnaarden/CPP_NETWORK_WRAPPER", -1, -1, 0);

		X509_set_issuer_name(x509, name);
		X509_sign(x509, pkey, EVP_sha1());

		// Set private key and certificate in SSL_CTX
		if (SSL_CTX_use_certificate(ctx, x509) != 1 || SSL_CTX_use_PrivateKey(ctx, pkey) != 1) {
			X509_free(x509);
			EVP_PKEY_free(pkey);
			SSL_CTX_free(ctx);
			return false;
		}

		// Optional: Verify private key matches the certificate
		if (SSL_CTX_check_private_key(ctx) != 1) {
			X509_free(x509);
			EVP_PKEY_free(pkey);
			SSL_CTX_free(ctx);
			return false;
		}

		// Clean up
		EVP_PKEY_free(pkey);
		X509_free(x509);

		return true;
	}

 	void TCPServer::StartListening(void (*callback)(TCPServer*, SOCKET, SSL*)) {
		while (true) {
			sockaddr_in ClientAddress;
			socklen_t ClientAddressSize = sizeof(ClientAddress);
			SOCKET ClientSocket = accept(m_Socket, (sockaddr*)&ClientAddress, &ClientAddressSize);
			if (ClientSocket <= 0) {
				//throw std::runtime_error("Failed to accept client");
				continue;
			}
			if (m_SSL) {
				SSL_CTX* ctx = SSL_CTX_new(SSLv23_server_method());
				if (Encrypt(ctx) != true) {
					std::cerr << "Failed to encrypt" << std::endl;
					continue;
				}
				SSL* ssl = SSL_new(ctx);
				SSL_set_fd(ssl, ClientSocket);
				if (SSL_accept(ssl) != 1) {
					std::cerr << "Failed to accept SSL connection" << std::endl;
					continue;
				}
				std::thread(callback, this, ClientSocket, ssl).detach();
			}
			else {
				std::thread(callback, this, ClientSocket, nullptr).detach();
			}
		}
	}
}
