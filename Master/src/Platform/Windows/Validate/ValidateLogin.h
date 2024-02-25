#pragma once
#include <Windows.h>
#include <lm.h>
#include <string>
#pragma comment(lib, "Netapi32.lib")

namespace Puppeteer {
	bool authenticateUser(std::string username, std::string password, std::string domain = "");
	bool userIsAdmin(std::string username);
}


