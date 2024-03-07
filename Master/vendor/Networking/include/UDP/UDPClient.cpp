#include "UDPClient.h"

namespace Networking {
	int g_UDPClientCount = 0;

	UDPClient::UDPClient(iProtocol iProt) {
		STARTWSA();
		m_Socket = socket(iProt, SOCK_DGRAM, IPPROTO_UDP);
		if (m_Socket == INVALID_SOCKET) {
			throw std::runtime_error("Failed to create socket");
			return;
		}
		g_UDPClientCount++;

		m_iProt = iProt;
	}

	int UDPClient::Send(const char* data, char* ip, UINT16 port, int length) {
		sockaddr_in ServerAddress{};
		ServerAddress.sin_family = m_iProt;
		ServerAddress.sin_port = htons(port);
		ServerAddress.sin_addr.s_addr = inet_addr(ip);

		size_t size = (length) ? length : strlen(data);
		std::string SizePacket = std::to_string(size);

		int ResultSize = sendto(m_Socket, SizePacket.c_str(), SizePacket.size(), 0, (sockaddr*)&ServerAddress, sizeof(ServerAddress));
		if (ResultSize == -1) {
			throw std::runtime_error("Failed to send size packet");
			return 1;
		}
		int TotalSent = 0;
		while (TotalSent < size) {
			int Sent = sendto(m_Socket, data + TotalSent, size - TotalSent, 0, (sockaddr*)&ServerAddress, sizeof(ServerAddress));
			if (Sent == -1) {
				throw std::runtime_error("Failed to send message");
				return 1;
			}
			TotalSent += Sent;
		}

		return 0;
	}

	int UDPClient::Receive(char*& data, sockaddr_in* ServerAddress) {
		socklen_t ServerAddressLength = sizeof(*ServerAddress);

		char SizePacket[20]; // 64-bit integer can be at most 20 characters long
		int ResultSize = recvfrom(m_Socket, (RECVFORM_BUFFER)SizePacket, 19, 0, (sockaddr*)ServerAddress, &ServerAddressLength);
		if (ResultSize == -1) {
			throw std::runtime_error("Failed to receive data");
			return 1;
		}
		SizePacket[ResultSize] = '\0';

		data = new char[std::stoi(SizePacket) + 1];
		int TotalReceived = 0;
		while (TotalReceived < std::stoi(SizePacket)) {
			int Received = recvfrom(m_Socket, (RECVFORM_BUFFER)data + TotalReceived, std::stoi(SizePacket) - TotalReceived, 0, (sockaddr*)&ServerAddress, &ServerAddressLength);
			if (Received == -1) {
				throw std::runtime_error("Failed to receive data");
				return 1;
			}
			TotalReceived += Received;
		}
		data[TotalReceived] = '\0';
		return 0;
	}

	UDPClient::~UDPClient() {
		closesocket(m_Socket);
		g_UDPClientCount--;
		if (g_UDPClientCount == 0) {
			WSACleanup();
		}
	}
}
