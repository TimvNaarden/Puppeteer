#include "pch.h"

namespace Puppeteer {
	Application* app = new Application({ "Puppeteer", 1920, 1080 });

	std::unordered_map<std::string, std::string> screenAction = { {"ActionType", "Screen"}, {"Action", "0"} };
	std::unordered_map<std::string, std::string> pcInfoAction = { {"ActionType", "ReqPCInfo"}, {"Action", "0"} };
	std::unordered_map<std::string, std::string> closeAction = { {"ActionType", "Close"}, {"Action", "0"} };
	std::unordered_map<std::string, std::string> lockInputAction = { {"ActionType", "LockInput"}, {"Action", "0"} };
	char* screenActionJson = "{\"ActionType\":\"Screen\",\"Action\":\"0\"}";
	char* pcInfoActionJson = "{\"ActionType\":\"ReqPCInfo\",\"Action\":\"0\"}";
	char* closeActionJson = "{\"ActionType\":\"Close\",\"Action\":\"0\"}";
	char* lockInputActionJson = "{\"ActionType\":\"LockInput\",\"Action\":\"0\"}";

	std::string Password;
	std::string Username;
	std::string Domain;
	std::string Ip;

	std::vector<PCInfo> PcInfos = {};


}