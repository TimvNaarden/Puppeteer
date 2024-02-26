#pragma once

#include "Socket/Socket.h"
#include "DirectX11/DirectX11.h"
#include "PCInfo/PCInfo.h"
#include "Json/ParseJson.h"
#include "Json/WriteJson.h"
#include "Validate/ValidateLogin.h"
#include "lz4.h"

#define PUPPET(x) std::cout << "Puppet: " << x << std::endl;
namespace Puppeteer {
	extern HHOOK g_keyboardHook;
	extern HHOOK g_mouseHook;

	LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
	
	class PClientSocket : public Socket {
		public:
			PClientSocket();
			~PClientSocket();
			void HandleServerSocket();
			void HandleClient(SOCKET clientSocket, SSL** pssl);
			int AcceptConnection(SOCKET clientsocket, SSL** pssl);
			int Listen(SOCKET clientSocket, SSL** pssl);

			DirectX11 m_Dx11;
			PCInfo m_PCInfo;
			bool m_Block;
	};

}