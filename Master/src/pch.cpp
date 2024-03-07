#include "pch.h"

namespace Puppeteer {
	Application* app = new Application({ "Puppeteer", 1920, 1080 });

	std::vector<PCInfo> PcInfos = {};
	std::string Ip = "";

	Credentials_T Credentials;
	Action_T Action = {};

}