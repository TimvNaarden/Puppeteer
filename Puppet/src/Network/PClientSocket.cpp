#include "PClientSocket.h"

namespace Puppeteer {
	enum class ActionType {
		Screen,
		ReqPCInfo,
		Close,
		LockInput,
		Credentials,
		Keystrokes,
		Mouse,
	};

	struct Credentials_T {
		char Username[256];
		char Password[256];
		char Domain[256];

		Credentials_T() {
			std::memset(Username, 0, sizeof(Username)); // Clear the memory
			std::memset(Password, 0, sizeof(Password)); // Clear the memory
			std::memset(Domain, 0, sizeof(Domain)); // Clear the memory
		}
	} creds;

	struct Action_T {
		ActionType Type;
		int dx;
		int dy;
		int Flags;
		int Inputdata;
	} Action;
	HHOOK g_keyboardHook;
	HHOOK g_mouseHook;

	LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
		return 1;
	}

	LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
		return 1;
	}

	Screencap m_Cap(GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
	PCInfo m_PCInfo;
	bool m_Block;

	int StartPuppetSocket(Networking::TCPServer* m_tcpServer) {
		std::fstream logFile{ "log.txt", std::ios::app };
		logFile << "Started listening"<< std::endl;
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

		memcpy(&creds, packet, sizeof(creds));


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
		if (!pssl) { PUPPET("Client Closed"); return; }
		if (!AcceptConnection(m_tcpServer, clientsocket, pssl)) { return; }

		while (true) {
			char* packet;
			if (m_tcpServer->Receive(clientsocket, packet, pssl)) {
				PUPPET("Client closed");
				BlockInput(false);
				if (g_keyboardHook != 0 && g_mouseHook != 0) {
					UnhookWindowsHookEx(g_keyboardHook);
					UnhookWindowsHookEx(g_mouseHook);
					PUPPET("Input unblocked")
				}
				return;
			}
			PUPPET(packet);

			memcpy(&Action, packet, sizeof(Action_T));

			if (Action.Type == ActionType::ReqPCInfo) {
				m_PCInfo.renew();
				std::string pcdata = WriteJson(m_PCInfo.toMap());
				char* pc = pcdata.data();
				if (m_tcpServer->Send(clientsocket, pc, 0, pssl)) {
					PUPPET("Failed to send PC Info");
				}
				else {
					PUPPET("PC Info sent");
				}
				continue;
			}
			else if (Action.Type == ActionType::Screen) {
				unsigned char* screenc = m_Cap.CaptureScreen(0,0);

				std::vector<char> compressedResults(LZ4_compressBound(m_Cap.GetSize()));
				int csize = LZ4_compress_fast((char*)screenc, compressedResults.data(), m_Cap.GetSize(), compressedResults.size(), 1);
				compressedResults.resize(csize);

				std::map<std::string, int> screen = {
					{"width", m_Cap.GetWidth()},
					{"height", m_Cap.GetHeight()},
					{"size", m_Cap.GetSize()},
					{"csize", csize}
				};
				std::string screendata = WriteJson(screen);
				char* screendatas = screendata.data();
				if (m_tcpServer->Send(clientsocket, screendatas, 0, pssl)) {
					PUPPET("Failed to send screen");
				}
				else {
					PUPPET("Screen sent");
				}
				char* screens = compressedResults.data();
				if (m_tcpServer->Send(clientsocket, screens, csize, pssl)) {
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
					BlockInput(true);
					g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
					if (!g_keyboardHook) {
						PUPPET("Couldn't block keyboard input");
					}

					g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
					if (!g_mouseHook) {
						PUPPET("Couldn't block mouse input");
					}
					PUPPET("Input blocked");
					
					
				}
				else {
					BlockInput(false);
					
					if (g_keyboardHook != 0 && g_mouseHook != 0) {
						UnhookWindowsHookEx(g_keyboardHook);
						UnhookWindowsHookEx(g_mouseHook);
					}
					PUPPET("Input unblocked");
					
				}
				continue;
			}
			else if (Action.Type == ActionType::Close) {
				PUPPET("Client closed");
				BlockInput(false);
				
				if (m_Block && g_keyboardHook != 0 && g_mouseHook != 0) {
					UnhookWindowsHookEx(g_keyboardHook);
					UnhookWindowsHookEx(g_mouseHook);
					PUPPET("Input unblocked")
				}
				
				delete[] packet;
				return;

			}
			delete[] packet;
		}
	}
}

