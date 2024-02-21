#pragma once
#include "Socket/Socket.h"
#include "Platform/Windows/DirectX11/DirectX11.h"
#include "Platform/Windows/PCInfo/PCInfo.h"
#include "Json/ParseJson.h"
#include "Json/WriteJson.h"
#include "Platform/Windows/Validate/ValidateLogin.h"


namespace Puppeteer
{

	class PClientSocket : public Socket
	{
	public:
		void HandleServerSocket();
		void HandleClient(SOCKET clientSocket);
		int AcceptConnection();
		int Listen(SOCKET clientSocket);
	private:
		DirectX11 m_Dx11;
		PCInfo m_PCInfo;
		bool m_Block;
	};

}
