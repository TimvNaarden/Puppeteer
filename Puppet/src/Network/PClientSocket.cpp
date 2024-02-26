#include "PClientSocket.h"


namespace Puppeteer
{
    HHOOK g_keyboardHook;
    HHOOK g_mouseHook;

    LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) { return 1; }
    
    LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) { return 1; }

    PClientSocket::PClientSocket() {
        // Init vars
        m_StopListen = 0;
        m_Socket = INVALID_SOCKET;
        m_Type = TCP;
        m_sslctx = nullptr;
        m_ssl = nullptr;
        m_CommunicationType = SERVER;
        m_Block = false;
        STARTWSA();
    }

    PClientSocket::~PClientSocket() {
        m_StopListen = 1;
        closesocket(m_Socket);
        WSACleanup();

        if (m_sslctx) {
            CleanupSSL();
        }
    }

    void PClientSocket::HandleServerSocket() {
        if (m_Type == UDP) {
            while (true) {
                Listen(m_Socket, &m_ssl);
            }
        }

        while (!m_StopListen) {
            sockaddr_in addr{};
            socklen_t addrlen = sizeof(addr);
            SOCKET clientSocket = accept(m_Socket, (struct sockaddr*)&addr, &addrlen);

            if (clientSocket == INVALID_SOCKET) {
                continue;
            }
            SSL* pssl = nullptr;
            if (m_SSLEnabled) {
                BIO* sbio = BIO_new_socket(clientSocket, BIO_NOCLOSE);
                EncryptSocket(&pssl);
                SSL_set_bio(pssl, sbio, sbio);

                int ret = SSL_accept(pssl);
                if (ret <= 0) {
                    PUPPET("Failed to accept SSL");
                    PUPPET(SSL_get_error(pssl, ret));
                    continue;
                }
            }
            std::thread t1 = std::thread(&Socket::HandleClient, this, clientSocket, &pssl);
            t1.detach();
        }
    }

    void PClientSocket::HandleClient(SOCKET clientSocket, SSL** pssl) {
        if (!AcceptConnection(clientSocket, pssl)) {
            closesocket(clientSocket);
            if (m_SSLEnabled) {
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

    int PClientSocket::AcceptConnection(SOCKET clientsocket, SSL** pssl)
    {
        int ret = 0;
        PUPPET(" Waiting for credentials");
        char* packet = ReceivePacket(clientsocket, pssl);
        if (packet == "Con Closed") {
			PUPPET("Client closed");
			return 0;
		}
        
        std::map<std::string, std::string> map = ParseJson<std::map<std::string, std::string>>(packet);

        std::string username = map["username"];
        std::string password = map["password"];
        std::string domain = map["domain"];

        if (authenticateUser(username, password, domain) && userIsAdmin(username, domain)) {
            ret = 1;

            if (SendPacket("Authenticated", pssl, NULL, clientsocket)) {
                PUPPET("Failed to send authentication");
            } else {
				PUPPET("Authenticated");
			}
		} else {
            if (SendPacket("Not Authenticated", pssl, NULL, clientsocket)) {
				PUPPET("Failed to send authentication");
			} else {
                PUPPET("Failed to authenticate");
            }
        }

        delete[] packet;

        return ret;
    }

    int PClientSocket::Listen(SOCKET clientSocket, SSL** pssl)
    {
        // Message format from master
        /*
        * {ActionType: "Action"         Possible Actions: ReqPCInfo, Screen, Keystrokes, Mouse       , LockInput, Close
        * Action: "Data"                Values:           0        , 0     , Keycodes  , Loc + Button, 0        , 0
        */
        if(!pssl) { PUPPET("Client Closed") return 1; }
        char* packet = ReceivePacket(clientSocket, pssl);
        if (packet == "Con Closed") {
			PUPPET("Client closed");
			return 1;
		}
        PUPPET(packet);
        std::map<std::string, std::string> Action = ParseJson<std::map<std::string, std::string>>(packet);

        if (Action["ActionType"] == "ReqPCInfo") {
            m_PCInfo.renew();
            if (SendPacket(WriteJson(m_PCInfo.toMap()).data(), pssl, NULL, clientSocket)) {
                PUPPET("Failed to send PC Info");
            }
            else {
                PUPPET("PC Info sent");
            }
        }
        else if (Action["ActionType"] == "Screen") {
            //No need to do anything regarding the fps as the master will handle it
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
            if (SendPacket(WriteJson(screen).data(), pssl, NULL, clientSocket)) {
                PUPPET("Failed to send screen");
            }
            else {
                PUPPET("Screen sent");
            }
            if (SendPacket(compressedResults.data(), pssl, csize, clientSocket)) {
                PUPPET("Failed to send screen data");
            }
            else {
                PUPPET("Screen data sent");
            }
            //CLIENTCMD(m_Dx11.getScreen());

        }
        else if (Action["ActionType"] == "Keystrokes") {
            //Data format from master: [{"Code" :, "Flags":}]
            std::vector<std::map<std::string, int>> keys = ParseJson<std::vector<std::map<std::string, int>>>(Action["Action"]);
            std::cout << keys.front()["Code"] << std::endl;
            const int keyCount = keys.size();
            INPUT* keyStrokes = new INPUT[keyCount];
            ZeroMemory(keyStrokes, sizeof(keyStrokes));
            for (int i = 0; i < keyCount; i++) {
                keyStrokes[i].type = INPUT_KEYBOARD;
                keyStrokes[i].ki.wVk = keys[i]["Code"];
                keyStrokes[i].ki.dwFlags = keys[i]["Flags"];
            }
            if (SendInput(keyCount, keyStrokes, sizeof(INPUT)) != keyCount) {
                PUPPET("Failed to send keystrokes");
            }
            else {
                PUPPET("Keystrokes sent");
            }
        }
        else if (Action["ActionType"] == "Mouse") {
            //Data format from master: [{"dx": , "dy", "mouseData": , "dwflags"}]
            std::vector<std::map<std::string, int>> mouseData = ParseJson<std::vector<std::map<std::string, int>>>(std::string(Action["Action"]));
            const int mouseCount = mouseData.size();
            INPUT* mouseAction = new INPUT[mouseCount];
            ZeroMemory(mouseAction, sizeof(mouseAction));
            for (int i = 0; i < mouseCount; i++) {
                mouseAction[i].type = INPUT_MOUSE;
                mouseAction[i].mi.dx = mouseData[i]["dx"];
                mouseAction[i].mi.dy = mouseData[i]["dy"];
                mouseAction[i].mi.mouseData = mouseData[i]["mouseData"];
                mouseAction[i].mi.dwFlags = mouseData[i]["dwFlags"];
                mouseAction[i].mi.time = 0;
            }
            if (SendInput(mouseCount, mouseAction, sizeof(INPUT)) != mouseCount) {
                PUPPET("Failed to send mouse");
            }
            else {
                PUPPET("Mouse sent");
            }

        }
        else if (Action["ActionType"] == "LockInput") {
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

        }
        else if (Action["ActionType"] == "Close" || packet == "Con Closed") {
            PUPPET("Client closed");
            if (m_Block) {
                BlockInput(FALSE);
                UnhookWindowsHookEx(g_keyboardHook);
                UnhookWindowsHookEx(g_mouseHook);
                PUPPET("Input " << "unblocked")
            }
            delete[] packet;
            return 1;

        }
        delete[] packet;
        // return 1 to break the loop and close the socket
        return 0;
    }

}
