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
#include "glad/glad.h"

namespace Puppeteer {
	extern std::string Error;
	extern bool modalOpen;
	extern Application* app;
	extern std::vector<PCInfo> PcInfos;
	extern std::string Ip;

	struct GridClient_T { 
		char Name[256]; 
		char Ip[256]; 
		GridClient_T() {
			std::memset(Name, 0, sizeof(Name)); 
			Name[255] = '\0';
			std::memset(Ip, 0, sizeof(Ip)); 
			Ip[255] = '\0';
		};
		GridClient_T(std::string input) {
			std::memset(Name, 0, sizeof(Name));
			Name[255] = '\0';
			std::memset(Ip, 0, sizeof(Ip)); 
			Ip[255] = '\0';
			int sep = input.find(",");
			std::string name = input.substr(0, sep);
			std::string ip = input.substr(sep + 1);
			std::memcpy(Name, name.c_str(), name.size());
			std::memcpy(Ip, ip.c_str(), ip.size());
		};
		std::string toString() {
			return std::string(Name) + "," + std::string(Ip);
		}
	};
	extern std::vector<GridClient_T> GridClients;

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
			std::memset(Username, 0, sizeof(Username)); 
			Username[255] = '\0';
			std::memset(Password, 0, sizeof(Password)); 
			Password[255] = '\0';
			std::memset(Domain, 0, sizeof(Domain)); 
			Domain[255] = '\0';
		}
	};
	extern Credentials_T Credentials;

	struct Action_T {
		ActionType Type;
		int dx;
		int dy;
		int Flags;
		int Inputdata;
	};
	extern Action_T Action;

	extern int LayerCount;
	extern int ActiveLayerIndex;

	typedef struct ImageData {
		int Width, Height;
		char* Texture;
		GLuint TextureID;
	};


	#define SAVE() {												\
		std::vector<std::string> GClients = {};						\
		for (GridClient_T Client : GridClients) {					\
			GClients.push_back(Client.toString());					\
		}															\
		std::vector<std::map<std::string, std::string>> pcs;		\
		for (PCInfo pc : PcInfos) {									\
			pcs.push_back(pc.toMap());								\
		}															\
		std::map<std::string, std::string> Save = {					\
			{"username", Credentials.Username},						\
			{"ip", Ip},												\
			{"domain", Credentials.Domain},							\
			{"pcs", WriteJson(pcs)},								\
			{"GridClients", WriteJson(GClients)}					\
		};															\
		OverrideJsonTable("Puppeteer", WriteJson(Save));			\
	}
}
