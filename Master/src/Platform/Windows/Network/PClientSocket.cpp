#include "pch.h"
#include "PClientSocket.h"

namespace Puppeteer {
	HHOOK g_keyboardHook;
	HHOOK g_mouseHook;

	LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) { return 1; }

	LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) { return 1; }

	DirectX11 m_Dx11;
	PCInfo m_PCInfo;
	bool m_Block;

	int StartPuppetSocket(Networking::TCPServer* m_tcpServer) {
		m_tcpServer->StartListening(Listen);
		return 0;
	}

	int AcceptConnection(Networking::TCPServer* m_tcpServer, SOCKET clientsocket, SSL* pssl) {
		int ret = 0;
		PUPPET(" Waiting for credentials");
		char* packet;
		if (m_tcpServer->Receive(clientsocket, packet, pssl)) {
			PUPPET("Client closed");
			return 0;
		}

		Credentials_T creds = *reinterpret_cast<Credentials_T*>(&packet);

		if (authenticateUser(creds.Username, creds.Password, creds.Domain) && userIsAdmin(creds.Username, creds.Domain)) {
			ret = 1;
			char* auth = "Authenticated";
			if (m_tcpServer->Send(clientsocket, auth, 0, pssl)) {
				PUPPET("Failed to send authentication");
			}
			else {
				PUPPET("Authenticated");
			}
		}
		else {
			char* auth = "Not Authenticated";
			if (m_tcpServer->Send(clientsocket, auth, 0, pssl)) {
				PUPPET("Failed to send authentication");
			}
			else {
				PUPPET("Failed to authenticate");
			}
		}

		delete[] packet;

		return ret;
	}

	void Listen(Networking::TCPServer* m_tcpServer, SOCKET clientsocket, SSL* pssl) {
		if (!pssl) { PUPPET("Client Closed") return; }
		if (!AcceptConnection(m_tcpServer, clientsocket, pssl)) { return; }

		while (true) {
			char* packet;
			if (m_tcpServer->Receive(clientsocket, packet, pssl) == -1) {
				PUPPET("Client closed");
				return;
			}
			PUPPET(packet);

			Action_T Action = *reinterpret_cast<Action_T*>(&packet);

			if (Action.Type == ActionType::ReqPCInfo) {
				m_PCInfo.renew();
				char* pcdata = WriteJson(m_PCInfo.toMap()).data();
				if (m_tcpServer->Send(clientsocket, pcdata, 0, pssl)) {
					PUPPET("Failed to send PC Info");
				}
				else {
					PUPPET("PC Info sent");
				}
				continue;
			}
			else if (Action.Type == ActionType::Screen) {
				screenCapture screencp = m_Dx11.getScreen();

				std::vector<char> compressedResults(LZ4_compressBound(screencp.size));
				int csize = LZ4_compress_fast((char*)Puppeteer::screenCapSubRes.pData, compressedResults.data(), screencp.size, compressedResults.size(), 1);
				compressedResults.resize(csize);

				std::map<std::string, int> screen = {
					{"width", screencp.width},
					{"height", screencp.height},
					{"size", screencp.size},
					{"csize", csize}
				};
				char* screendata = WriteJson(screen).data();
				if (m_tcpServer->Send(clientsocket, screendata, 0, pssl)) {
					PUPPET("Failed to send screen");
				}
				else {
					PUPPET("Screen sent");
				}
				char* screens = compressedResults.data();
				if (m_tcpServer->Send(clientsocket, screens, 0, pssl)) {
					PUPPET("Failed to send screen data");
				}
				else {
					PUPPET("Screen data sent");
				}
				continue;
			}
			else if (Action.Type == ActionType::Keystrokes) {
				INPUT* keyStrokes = new INPUT[1];
				ZeroMemory(keyStrokes, sizeof(keyStrokes));
				keyStrokes[0].type = INPUT_KEYBOARD;
				keyStrokes[0].ki.wVk = Action.Inputdata;
				keyStrokes[0].ki.dwFlags = Action.Flags;

				if (SendInput(1, keyStrokes, sizeof(INPUT)) != 1) {
					PUPPET("Failed to send keystrokes");
				}
				else {
					PUPPET("Keystrokes sent");
				}
				continue;
			}
			else if (Action.Type == ActionType::Mouse) {
				INPUT* mouseAction = new INPUT[1];
				ZeroMemory(mouseAction, sizeof(mouseAction));

				mouseAction[0].type = INPUT_MOUSE;
				mouseAction[0].mi.dx = Action.dx;
				mouseAction[0].mi.dy = Action.dy;
				mouseAction[0].mi.mouseData = Action.Inputdata;
				mouseAction[0].mi.dwFlags = Action.Flags;
				mouseAction[0].mi.time = 0;

				if (SendInput(1, mouseAction, sizeof(INPUT)) != 1) {
					PUPPET("Failed to send mouse");
				}
				else {
					PUPPET("Mouse sent");
				}
				continue;
			}
			else if (Action.Type == ActionType::LockInput) {
				m_Block = !m_Block;
				if (m_Block) {
					BlockInput(TRUE);
					g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
					if (!g_keyboardHook) {
						//UnhookWindowsHookEx(g_keyboardHook);
						PUPPET("Couldn't block keyboard input");
					}

					// Install mouse hook
					g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
					if (!g_mouseHook) {
						//UnhookWindowsHookEx(g_mouseHook);
						PUPPET("Couldn't block mouse input");
					}
					PUPPET("Input " << "blocked");
				}
				else {
					BlockInput(FALSE);
					UnhookWindowsHookEx(g_keyboardHook);
					UnhookWindowsHookEx(g_mouseHook);
					PUPPET("Input " << "unblocked");
				}
				continue;
			}
			else if (Action.Type == ActionType::Close) {
				PUPPET("Client closed");
				if (m_Block) {
					BlockInput(FALSE);
					UnhookWindowsHookEx(g_keyboardHook);
					UnhookWindowsHookEx(g_mouseHook);
					PUPPET("Input " << "unblocked")
				}
				delete[] packet;
				return;

			}
			delete[] packet;
		}
	}
}

