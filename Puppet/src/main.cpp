#include "Network/PClientSocket.h"

namespace Puppeteer
{
	static int RunPuppet() {
		Puppeteer::PClientSocket client;
		client.Create(IPV4, TCP, SERVER, 54000, "192.168.2.62", true);
		client.HandleServerSocket();
		return 0;
	}
}


int main() {
	return Puppeteer::RunPuppet();
}
