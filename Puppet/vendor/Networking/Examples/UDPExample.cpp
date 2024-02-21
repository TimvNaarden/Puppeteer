#include "Socket/Socket.h"

// Samle Server Configuration for udp
static int Server()
{
	Socket server;
	if (server.Create(IPV4, UDP, SERVER, 54000))
	{
		return 1;
	}

	server.HandleServerSocket();

	return 0;

}

//Sample Client Configuration for udp
static int Client() {
	Socket client;

	if (client.Create(IPV4, UDP, CLIENT))
	{
		CLIENTCMD("Failed to create socket!");
		return 1;
	}
	else
	{
		CLIENTCMD("Connected");
	}

	while (true)
	{
		SOCKADDR_IN dest;
		dest.sin_family = AF_INET;
		dest.sin_port = htons(54000);
		dest.sin_addr.s_addr = inet_addr("127.0.0.1");
		int result = client.SendPacket("Hello World", 0, (SOCKADDR*)&dest);

		if (result == -1) {
			CLIENTCMD("Connection Closed");
			return 1;
		}
		else if (result == 1)
		{
			CLIENTCMD("Failed to send packet!");
			continue;
		}
		else
		{
			CLIENTCMD("Packet sent!");
		}
	}
}
int main()
{
	std::thread serverThread(Server);
	serverThread.detach();
	return Client();

}

