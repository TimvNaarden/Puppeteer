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
	extern std::vector<PCInfo> PcInfos;
	extern std::string Ip;

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
			Username[255] = '\0';
			std::memset(Password, 0, sizeof(Password)); // Clear the memory
			Password[255] = '\0';
			std::memset(Domain, 0, sizeof(Domain)); // Clear the memory
			Domain[255] = '\0';
		}
	};

	struct Action_T {
		ActionType Type;
		int dx;
		int dy;
		int Flags;
		int Inputdata;
	};

	extern Credentials_T Credentials;
	extern Action_T Action;
}
