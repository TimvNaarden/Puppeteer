#pragma once
#include "TCP/TCPServer.h"
#include "BitBlt/Screencap.h"
#include "PCInfo/PCInfo.h"
#include "Json/ParseJson.h"
#include "Json/WriteJson.h"
#include "Validate/ValidateLogin.h"
#include "DirectX11/DirectX11.h"
#include <lz4.h>
#include <string>
#include <array>


#define PUPPET(x) std::cout << "Puppet: " << x << std::endl;

extern std::fstream logFile;
extern DWORD g_dwSessionID;
namespace Puppeteer {
	extern HHOOK g_keyboardHook;
	extern HHOOK g_mouseHook;

	LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);

	int StartPuppetSocket(Networking::TCPServer* m_tcpServer);
	int AcceptConnection(Networking::TCPServer* m_tcpServer, SOCKET clientsocket, SSL* pssl);
	void Listen(Networking::TCPServer* m_tcpServer, SOCKET clientsocket, SSL* pssl);

	extern Screencap m_Cap;
	extern PCInfo m_PCInfo;
	extern DirectX11 m_dx11;
	extern bool m_Block;

}