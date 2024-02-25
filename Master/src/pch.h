#pragma once
#include "winsock2.h"

// Utilities
#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

// Data Types
#include <array>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <Core/Application.h>
#include <Platform/Windows/PCInfo/PCInfo.h>

namespace Puppeteer {
	extern Application* app;

	extern std::unordered_map<std::string, std::string> screenAction;
	extern std::unordered_map<std::string, std::string> pcInfoAction;
	extern std::unordered_map<std::string, std::string> closeAction;
	extern std::unordered_map<std::string, std::string> lockInputAction;
	extern char* screenActionJson;
	extern char* pcInfoActionJson;
	extern char* closeActionJson;
	extern char* lockInputActionJson;

	extern std::string Password;
	extern std::string Username;
	extern std::string Domain;
	extern std::string Ip;

	extern std::vector<PCInfo> PcInfos;
}