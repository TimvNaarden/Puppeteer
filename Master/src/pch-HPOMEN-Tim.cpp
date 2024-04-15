#include "pch.h"

namespace Puppeteer {
	Application* app = new Application({ "Puppeteer", 1920, 1080 });

	std::string Error = "";
	bool modalOpen = false;

	std::vector<PCInfo> PcInfos = {};
	std::string Ip = "";
	
	std::vector<GridClient_T> GridClients = {};

	Credentials_T Credentials;
	
	Action_T Action = {};

	int LayerCount = 0;
	int ActiveLayerIndex = 0;
}