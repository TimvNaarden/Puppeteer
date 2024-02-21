#include "pch.h"
#include "PClientSocket.h"

namespace Puppeteer
{
    void PClientSocket::HandleServerSocket()
    {
        if (m_Type == UDP) {
            while (true) {
                Listen(m_Socket);
            }
        }

        while (!m_StopListen) {
            sockaddr_in addr{};
            socklen_t addrlen = sizeof(addr);
            SOCKET clientSocket = accept(m_Socket, (struct sockaddr*)&addr, &addrlen);

            if (clientSocket == INVALID_SOCKET) {
                continue;
            }

            if (m_ssl) {
                BIO* sbio = BIO_new_socket(clientSocket, BIO_NOCLOSE);
                SSL_set_bio(m_ssl, sbio, sbio);

                int ret = SSL_accept(m_ssl);
                if (ret <= 0) {
                    SERVERCMD("Failed to accept SSL");
                    SERVERCMD(SSL_get_error(m_ssl, ret));
                    continue;
                }
            }
            std::thread t1 = std::thread(&Socket::HandleClient, this, clientSocket);
            t1.detach();
        }
    }

    void PClientSocket::HandleClient(SOCKET clientSocket)
    {
        if (!AcceptConnection()) {
            closesocket(clientSocket);
            return;
        }

        while (true) {
            if (Listen(clientSocket)) {
                break;
            }
        }
        closesocket(clientSocket);
    }

    int PClientSocket::AcceptConnection()
    {
        int ret = 0;
        char* packet = ReceivePacket();
        std::map<std::string, std::string> map = ParseJson<std::map<std::string, std::string>>(packet);

        std::string username = map["username"];
        std::string password = map["password"];
        std::string domain = map["domain"];

        delete &password;

        if (authenticateUser(username, password, domain) && userIsAdmin(username)) {
            ret = 1;
        }

        free(packet);

        return ret;
    }

    int PClientSocket::Listen(SOCKET clientSocket)
    {
        // Message format from master
        /*
        * {ActionType: "Action"         Possible Actions: ReqPCInfo, Screen, Keystrokes, Mouse       , LockInput, Close
        * Action: "Data"                Values:           0        , 0     , Keycodes  , Loc + Button, 0        , 0
        */
        char* packet = ReceivePacket(clientSocket);
        std::map<std::string, std::string> Action = ParseJson<std::map<std::string, std::string>>(packet);

        if (Action["ActionType"] == "ReqPCInfo") {
            m_PCInfo.renew();
            if (SendPacket(WriteJson(m_PCInfo.toMap()).data())) {
                CLIENTCMD("Failed to send PC Info");
            }
        }
        else if (Action["ActionType"] == "Screen") {
            //No need to do anything regardint the fps as the master will handle it
            if (SendPacket(WriteJson(m_Dx11.getScreen()).data())) {
                CLIENTCMD("Failed to send screen");
            }
        }
        else if (Action["ActionType"] == "Keystrokes") {
            //Data format from master: [Code, code, code....]
            std::vector<int> keys = ParseJson<std::vector<int>>(Action["Action"]);
            const int keyCount = keys.size();
            INPUT* keyStrokes = new INPUT[keyCount];
            for (int i = 0; i < keyCount; i++) {
                keyStrokes[i].type = INPUT_KEYBOARD;
                keyStrokes[i].ki.wVk = keys[i];
            }
            for (int i = keyCount - 1; i >= 0; i--) {
                keyStrokes[i].type = INPUT_KEYBOARD;
                keyStrokes[i].ki.wVk = keys[i];
                keyStrokes[i].ki.dwFlags = KEYEVENTF_KEYUP;
            }
            if (SendInput(keys.size(), keyStrokes, sizeof(INPUT)) != 0) {
                CLIENTCMD("Failed to send keystrokes");
            }
        }
        else if (Action["ActionType"] == "Mouse") {
            //Data format from master: [dx, dy, mouseData, dwflags]
            std::vector<double> mouseData = ParseJson<std::vector<double>>(std::string(Action["Action"]));
            INPUT mouseAction{};
            mouseAction.type = INPUT_MOUSE;
            mouseAction.mi.dx = mouseData[0];
            mouseAction.mi.dy = mouseData[1];
            mouseAction.mi.mouseData = mouseData[2];
            mouseAction.mi.dwFlags = mouseData[3];
            if (SendInput(1, &mouseAction, sizeof(INPUT)) != 0) {
                CLIENTCMD("Failed to send mouse");
            }

        }
        else if (Action["ActionType"] == "LockInput") {
            m_Block = !m_Block;
            if (BlockInput(m_Block) != 0) {
                CLIENTCMD("Failed to block input");
            }
        }
        else if (Action["ActionType"] == "Close") {
            //Make sure that the user can use the computer again
            BlockInput(FALSE);
            //Server wanted to close so we don't need to send anything
            return 1;

        }

        // return 1 to break the loop and close the socket
        return 0;
    }

 }

