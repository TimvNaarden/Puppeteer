#include "PClientSocket.h"

namespace Puppeteer
{
    // Global variables for hook handles
    HHOOK g_keyboardHook;
    HHOOK g_mouseHook;

    // Keyboard hook procedure
    LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        // Block keyboard input
        return 1;
    }

    // Mouse hook procedure
    LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
        // Block mouse input
        return 1;
    }

    PClientSocket::PClientSocket()
    {
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

    PClientSocket::~PClientSocket()
    {
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
                    SERVERCMD("Failed to accept SSL");
                    SERVERCMD(SSL_get_error(pssl, ret));
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
        SERVERCMD(" Waiting for credentials");
        char* packet = ReceivePacket(clientsocket,pssl);
        SERVERCMD(packet);
        std::map<std::string, std::string> map = ParseJson<std::map<std::string, std::string>>(packet);

        std::string username = map["username"];
        std::string password = map["password"];
        std::string domain = map["domain"];
       
        if (authenticateUser(username, password, domain) && userIsAdmin(username)) {
           ret = 1;

           if (SendPacket("Authenticated", pssl, NULL, clientsocket)) {
               SERVERCMD("Failed to send authentication");
           }
        }

        //free(packet);

        return ret;
    }

    int PClientSocket::Listen(SOCKET clientSocket, SSL** pssl)
    {
        // Message format from master
        /*
        * {ActionType: "Action"         Possible Actions: ReqPCInfo, Screen, Keystrokes, Mouse       , LockInput, Close
        * Action: "Data"                Values:           0        , 0     , Keycodes  , Loc + Button, 0        , 0
        */
        char* packet = ReceivePacket(clientSocket, pssl);
        CLIENTCMD (packet);
        std::map<std::string, std::string> Action = ParseJson<std::map<std::string, std::string>>(packet);

        if (Action["ActionType"] == "ReqPCInfo") {
            m_PCInfo.renew();
            if (SendPacket(WriteJson(m_PCInfo.toMap()).data(), pssl, NULL, clientSocket)) {
                CLIENTCMD("Failed to send PC Info");
            }
            else {
                CLIENTCMD("PC Info sent");
            }
        }
        else if (Action["ActionType"] == "Screen") {
            //No need to do anything regarding the fps as the master will handle it
            screenCapture screencp = m_Dx11.getScreen();
            char* data = new char[screencp.size];
            memcpy(data, Puppeteer::screenCapSubRes.pData, screencp.size);
            std::map<std::string, std::string> screen = { 
                {"width", std::to_string(screencp.width)}, 
                {"height", std::to_string(screencp.height)}, 
				{"size", std::to_string(screencp.size)}
               };
            if (SendPacket(WriteJson(screen).data(), pssl, NULL, clientSocket)) {
                CLIENTCMD("Failed to send screen");
            }
            else {
                CLIENTCMD("Screen sent");
            }
            if (SendPacket(data, pssl, screencp.size, clientSocket)) {
                CLIENTCMD("Failed to send screen data");
            }
            else {
				CLIENTCMD("Screen data sent");
			}
            delete[] data;
            //CLIENTCMD(m_Dx11.getScreen());

        }
        else if (Action["ActionType"] == "Keystrokes") {
            //Data format from master: [{"Code" :, "Flags":}]
            //Be aware, each key is pressed and when each key is pressend the get released in reverse order
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
            if (SendInput(keyCount , keyStrokes, sizeof(INPUT)) != keyCount) {
                CLIENTCMD("Failed to send keystrokes");
            }
            else {
                CLIENTCMD("Keystrokes sent");
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
			}
            if (SendInput(mouseCount, mouseAction, sizeof(INPUT)) != mouseCount) {
                CLIENTCMD("Failed to send mouse");
            }
            else {
				CLIENTCMD("Mouse sent");
			}

        }
        else if (Action["ActionType"] == "LockInput") {
            m_Block = !m_Block;
            if (m_Block) {
                g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
                if (!g_keyboardHook) {
                    //UnhookWindowsHookEx(g_keyboardHook);
                    CLIENTCMD("Couldn't block keyboard input"); 
                }

                // Install mouse hook
                g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
                if (!g_mouseHook) {
                    //UnhookWindowsHookEx(g_mouseHook);
                    CLIENTCMD("Couldn't block mouse input");
                }
                CLIENTCMD("Input " << "blocked");
            }
            else {
				UnhookWindowsHookEx(g_keyboardHook);
				UnhookWindowsHookEx(g_mouseHook);
                CLIENTCMD("Input " << "unblocked");
			}

        }
        else if (Action["ActionType"] == "Close") {
            CLIENTCMD("Client closed");
            if (m_Block) {
                UnhookWindowsHookEx(g_keyboardHook);
                UnhookWindowsHookEx(g_mouseHook);
                CLIENTCMD("Input " << "unblocked")
            }
            return 1;

        }

        // return 1 to break the loop and close the socket
        return 0;
    }

 }
