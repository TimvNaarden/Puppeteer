#include "PCInfo.h"

namespace Puppeteer {
	IWbemLocator* m_pLocator;
	IWbemServices* m_pService;
	IWbemServices* m_pServiceDisk;
	int PCInfoCounter = 0;

	PCInfo::PCInfo(bool GetLocal) {
		//Initialize VARS
		m_CPUName = "";
		m_Cores = 0;
		m_ThreadCount = 0;
		m_BaseClockSpeed = 0;
		
		m_Mob = "";

		m_Systemname = "";

		m_MemoryCapacity = 0;
		m_MemorySpeed = 0;
		m_MemoryType = "";
		m_MemoryDimms = 0;

		m_MACAddres = {};
		m_StorageNames = {};
		m_DiskSize = {};
		m_DiskType = {};
		m_VideoName = {};
		m_Vram = {};

		m_hr = S_OK;

		if (PCInfoCounter == 0) init();
		PCInfoCounter++;
		if(GetLocal) renew();
	}

	PCInfo::PCInfo() {
		//Initialize VARS
		m_CPUName = "";
		m_Cores = 0;
		m_ThreadCount = 0;
		m_BaseClockSpeed = 0;

		m_Mob = "";

		m_Systemname = "";

		m_MemoryCapacity = 0;
		m_MemorySpeed = 0;
		m_MemoryType = "";
		m_MemoryDimms = 0;

		m_MACAddres = {};
		m_StorageNames = {};
		m_DiskSize = {};
		m_DiskType = {};
		m_VideoName = {};
		m_Vram = {};

		m_hr = S_OK;

		if (PCInfoCounter == 0) init();
		PCInfoCounter++;
		renew();
	}

	PCInfo::PCInfo(std::map<std::string, std::string> map) {
		if (PCInfoCounter == 0) init();
		PCInfoCounter++;
		m_CPUName = map["CPUName"];
		m_Cores = std::stoi(map["Cores"]);
		m_ThreadCount = std::stoi(map["ThreadCount"]);
		m_BaseClockSpeed = std::stoi(map["BaseClockSpeed"]);

		m_Mob = map["Mob"];

		m_Systemname = map["Systemname"];

		m_MemoryCapacity = std::stoull(map["MemoryCapacity"]);
		m_MemorySpeed = std::stoi(map["MemorySpeed"]);
		m_MemoryType = map["MemoryType"];		
		m_MemoryDimms = std::stoi(map["MemoryDimms"]);

		m_MACAddres = ParseJson<std::vector<std::string>>(map["MACAddres"]);

		m_StorageNames = ParseJson<std::vector<std::string>>(map["StorageNames"]);
		m_DiskSize = ParseJson<std::vector<UINT64>>(map["DiskSize"]);
		m_DiskType = ParseJson<std::vector<std::string>>(map["DiskType"]);

		m_VideoName = ParseJson<std::vector<std::string>>(map["VideoName"]);
		m_Vram = ParseJson<std::vector<UINT32>>(map["Vram"]);

		m_hr = S_OK;
	}

	PCInfo::~PCInfo() {
		PCInfoCounter--;
		if (PCInfoCounter != 0) return;

		m_pService->Release();
		m_pLocator->Release();
		CoUninitialize();
	}

	std::map<std::string, std::string> PCInfo::toMap() {

		std::map<std::string, std::string> map;
		map["CPUName"] = m_CPUName;
		map["Cores"] = std::to_string(m_Cores);
		map["ThreadCount"] = std::to_string(m_ThreadCount);
		map["BaseClockSpeed"] = std::to_string(m_BaseClockSpeed);

		map["Mob"] = m_Mob;

		map["Systemname"] = m_Systemname;

		map["MemoryCapacity"] = std::to_string(m_MemoryCapacity);
		map["MemorySpeed"] = std::to_string(m_MemorySpeed);
		map["MemoryType"] = m_MemoryType;
		map["MemoryDimms"] = std::to_string(m_MemoryDimms);


		map["MACAddres"] = WriteJson(m_MACAddres);

		map["StorageNames"] = WriteJson(m_StorageNames);
		map["DiskSize"] = WriteJson(m_DiskSize);
		map["DiskType"] = WriteJson(m_DiskType);

		map["VideoName"] = WriteJson(m_VideoName);
		map["Vram"] = WriteJson(m_Vram);
		return map;
	}

	void PCInfo::renew() {
		// reset all values
		m_CPUName = "";
		m_Cores = 0;
		m_ThreadCount = 0;
		m_BaseClockSpeed = 0;

		m_Mob = "";

		m_Systemname = "";

		m_MemoryCapacity = 0;
		m_MemorySpeed = 0;
		m_MemoryType = "";
		m_MemoryDimms = 0;

		m_MACAddres = {};
		m_StorageNames = {};
		m_DiskSize = {};
		m_DiskType = {};
		m_VideoName = {};
		m_Vram = {};

		VARIANT vtProp = {};
		IEnumWbemClassObject* pEnumerator = NULL;
		IWbemClassObject* clsObject = 0;
		ULONG returned = 0;

		m_hr = m_pService->ExecQuery(L"WQL", L"SELECT * FROM Win32_Processor", WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
		if (FAILED(m_hr)) {
			PCINFO("Query for processor failed");
		}
		else {
			m_hr = pEnumerator->Next(0, 1, &clsObject, &returned);
			if (FAILED(m_hr)) {
				PCINFO("Failed iteration");
				clsObject->Release();
				pEnumerator->Release();
			}
			else {
				m_hr = clsObject->Get(L"Name", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) {
					PCINFO("Failed to get processor name");
					VariantClear(&vtProp);
				}
				else {
				m_CPUName = BstrToStdString(vtProp.bstrVal);
					VariantClear(&vtProp);
				}

				m_hr = clsObject->Get(L"SystemName", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) {
					PCINFO("Failed to get system name");
					VariantClear(&vtProp);
				}
				else {
					m_Systemname = BstrToStdString(vtProp.bstrVal);
					VariantClear(&vtProp);
				}

				m_hr = clsObject->Get(L"ThreadCount", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) {
					PCINFO("Failed to get amount of threads");
					VariantClear(&vtProp);
				}
				else {
					m_ThreadCount = vtProp.intVal;
					VariantClear(&vtProp);
				}


				m_hr = clsObject->Get(L"NumberOfCores", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) {
					PCINFO("Failed to get number of cores");
					VariantClear(&vtProp);
				}
				else {
					m_Cores = vtProp.intVal;
					VariantClear(&vtProp);
				}

				m_hr = clsObject->Get(L"MaxClockSpeed", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) {
					PCINFO("Failed to get max clock speed");
					VariantClear(&vtProp);
				}
				else {
					m_BaseClockSpeed = vtProp.intVal;
					VariantClear(&vtProp);
				}

				clsObject->Release();
				pEnumerator->Release();
			}
		}

		m_hr = m_pService->ExecQuery(L"WQL", L"SELECT * FROM Win32_BaseBoard", WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
		if (FAILED(m_hr)) {
			PCINFO("Query for motherboard failed");
		}
		else {
			m_hr = pEnumerator->Next(WBEM_INFINITE, 1, &clsObject, &returned);
			if (FAILED(m_hr)) {
				PCINFO("Failed iteration");
				clsObject->Release();
				pEnumerator->Release();
			}
			else {
				m_hr = clsObject->Get(L"Manufacturer", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) {
					PCINFO("Failed to get motherboard manufacturer");
					VariantClear(&vtProp);
				}
				else {
					m_Mob = BstrToStdString(vtProp.bstrVal);
					VariantClear(&vtProp);
					m_Mob += " ";
				}

				m_hr = clsObject->Get(L"Product", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) {
					PCINFO("Failed to get motherboard name");
					VariantClear(&vtProp);
				}
				else {
					m_Mob += BstrToStdString(vtProp.bstrVal);
					VariantClear(&vtProp);
				}

				clsObject->Release();
				pEnumerator->Release();
			}
		}

		m_hr = m_pService->ExecQuery(L"WQL", L"SELECT * FROM Win32_VideoController", WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
		if (FAILED(m_hr)) {
			PCINFO("Query for video failed");
		}
		else {
			while ((int)(m_hr = pEnumerator->Next(WBEM_INFINITE, 1, &clsObject, &returned)) == (int)WBEM_S_NO_ERROR) {
				m_hr = clsObject->Get(L"Name", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) {
					PCINFO("Failed to get video name");
					VariantClear(&vtProp);
				}
				else {
					m_VideoName.push_back(BstrToStdString(vtProp.bstrVal));
					VariantClear(&vtProp);
				}

				m_hr = clsObject->Get(L"AdapterRAM", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) {
					PCINFO("Failed to get video ram");
					VariantClear(&vtProp);
				}
				else {
					m_Vram.push_back(vtProp.lVal);
					VariantClear(&vtProp);
				}

			}
			clsObject->Release();
			pEnumerator->Release();

		}

		m_hr = m_pService->ExecQuery(L"WQL", L"SELECT * FROM Win32_NetworkAdapterConfiguration", WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
		if (FAILED(m_hr)) {
			PCINFO("Query for network failed");
			pEnumerator->Release();
		}
		else {
			while ((int)(m_hr = pEnumerator->Next(WBEM_INFINITE, 1, &clsObject, &returned)) == (int)WBEM_S_NO_ERROR) {
				m_hr = clsObject->Get(L"MACAddress", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) {
					PCINFO("Failed to get mac address");
					VariantClear(&vtProp);
				}
				else {
					if (vtProp.bstrVal != L"") {
						m_MACAddres.push_back(BstrToStdString(vtProp.bstrVal));
						VariantClear(&vtProp);
					}
				}

				clsObject->Release();
			}
			pEnumerator->Release();
		}

		m_hr = m_pServiceDisk->ExecQuery(L"WQL", L"SELECT * FROM MSFT_PhysicalDisk", WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
		if (FAILED(m_hr)) {
			PCINFO("Failed to open namespace");

		}
		else {
			while ((int)(m_hr = pEnumerator->Next(WBEM_INFINITE, 1, &clsObject, &returned)) == (int)WBEM_S_NO_ERROR) {
				m_hr = clsObject->Get(L"Size", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) {
					PCINFO("Failed to get storage size");
					VariantClear(&vtProp);
				}
				else {
					m_DiskSize.push_back(std::stoull(vtProp.bstrVal));
					VariantClear(&vtProp);
				}

				m_hr = clsObject->Get(L"FriendlyName", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) {
					PCINFO("Failed to get storage name");
					VariantClear(&vtProp);
				}
				else {
					m_StorageNames.push_back(BstrToStdString(vtProp.bstrVal));
					VariantClear(&vtProp);
				}

				m_hr = clsObject->Get(L"MediaType", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) {
					PCINFO("Failed to get storage type");
					VariantClear(&vtProp);
				}
				else {
					m_DiskType.push_back(getMediaType(vtProp.intVal));
					VariantClear(&vtProp);
				}
				clsObject->Release();
			}
			pEnumerator->Release();
		}

		std::vector<MemoryInformation*> memoryInformation = getMemoryInformation();
		for (auto& memoryInformation1 : memoryInformation) {
			if (memoryInformation1->size == 32767) {
				m_MemoryCapacity += memoryInformation1->extendedSize;
			}
			else {
				m_MemoryCapacity += memoryInformation1->size;
			}
			m_MemoryDimms++;
		}
		if (memoryInformation.size() == 0) return;
		m_MemorySpeed = memoryInformation.front()->speed;
		m_MemoryType = getMemoryType(memoryInformation.front()->memoryType);
	}

	std::string PCInfo::BstrToStdString(BSTR bstr) {
		if (!bstr) return "";

		int len = SysStringLen(bstr);
		std::wstring wideStr(bstr, len);

		int size_needed = WideCharToMultiByte(CP_ACP, 0, &wideStr[0], len, nullptr, 0, nullptr, nullptr);
		std::string result(size_needed, 0);
		WideCharToMultiByte(CP_ACP, 0, &wideStr[0], len, &result[0], size_needed, nullptr, nullptr);

		return result;
	}

	std::string PCInfo::getMemoryType(BYTE b) {
		switch (b) {
		case 0x01:
			return "Other";
		case 0x02:
			return "Unknown";
		case 0x03:
			return "DRAM";
		case 0x04:
			return "EDRAM";
		case 0x05:
			return "VRAM";
		case 0x06:
			return "SRAM";
		case 0x07:
			return "RAM";
		case 0x08:
			return "ROM";
		case 0x09:
			return "FLASH";
		case 0x0A:
			return "EEPROM";
		case 0x0B:
			return "FEPROM";
		case 0x0C:
			return "EPROM";
		case 0x0D:
			return "CDRAM";
		case 0x0E:
			return "3DRAM";
		case 0x0F:
			return "SDRAM";
		case 0x10:
			return "SGRAM";
		case 0x11:
			return "RDRAM";
		case 0x12:
			return "DDR";
		case 0x13:
			return "DDR2";
		case 0x14:
			return "DDR2 FB-DIMM";
		case 0x18:
			return "DDR3";
		case 0x19:
			return "FBD2";
		case 0x1A:
			return "DDR4";
		case 0x1B:
			return "LPDDR";
		case 0x1C:
			return "LPDDR2";
		case 0x1D:
			return "LPDDR3";
		case 0x1E:
			return "LPDDR4";
		case 0x1F:
			return "Logical non-volatile device";
		case 0x20:
			return "HBM";
		case 0x21:
			return "HBM2";
		case 0x22:
			return "DDR5";
		case 0x23:
			return "LPDDR5";
		case 0x24:
			return "HBM3";
		default:
			return "Unknown";
		};
	}

	std::string PCInfo::getMediaType(int i) {
		switch (i) {
		case 0:
			return "Unspecified";
		case 3:
			return "HDD";
		case 4:
			return "SSD";
		case 5:
			return "SCM";
		default:
			return "Unknown";
		};
	}

	std::vector<PCInfo::MemoryInformation*> PCInfo::getMemoryInformation() {
		std::vector<MemoryInformation*> smBiosDataVec = {};
		RawSMBIOSData* smBiosData = nullptr;

		DWORD smBiosDataSize = GetSystemFirmwareTable('RSMB', 0, NULL, 0);

		if (smBiosDataSize == 0) {
			PCINFO("Failed to get SMBIOS data size.");
			return smBiosDataVec;
		}

		smBiosData = (RawSMBIOSData*)HeapAlloc(GetProcessHeap(), 0, smBiosDataSize);

		DWORD bytesRetrieved = GetSystemFirmwareTable('RSMB', 0, smBiosData, smBiosDataSize);

		if (bytesRetrieved != smBiosDataSize) {
			PCINFO("Failed to get SMBIOS data.");
			HeapFree(GetProcessHeap(), 0, smBiosData);
			return smBiosDataVec;
		}

		BYTE* data = smBiosData->SMBIOSTableData;

		while (data < smBiosData->SMBIOSTableData + smBiosData->Length) {
			BYTE* next;
			SMBIOSHEADER* header = (SMBIOSHEADER*)data;

			if (header->length < 4)	break;


			if (header->type == 0x11 && header->length >= 0x19)  smBiosDataVec.push_back((MemoryInformation*)header);
			

			next = data + header->length;

			while (next < smBiosData->SMBIOSTableData + smBiosData->Length && (next[0] != 0 || next[1] != 0)) {
				next++;
			}
			next += 2;

			data = next;

		}

		HeapFree(GetProcessHeap(), 0, smBiosData);
		return smBiosDataVec;
	}

	void PCInfo::init()
	{
		m_hr = CoInitializeEx(0, COINIT_MULTITHREADED);
		if (FAILED(m_hr)) {
			PCINFO("Failed to initialize COM library");
			return;
		}

		m_hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0);
		if (FAILED(m_hr)) {
			PCINFO("Failed to initialize security");

		}

		m_hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&m_pLocator);
		if (FAILED(m_hr)) {
			PCINFO("Failed to create IWbemLocator object");
			CoUninitialize();
			return;
		}


		m_hr = m_pLocator->ConnectServer(L"root\\CIMV2", NULL, NULL, 0, NULL, 0, 0, &m_pService);
		if (FAILED(m_hr)) {
			PCINFO("Could not connect");
			m_pLocator->Release();
			CoUninitialize();
			return;
		}


		m_hr = m_pLocator->ConnectServer(L"root\\microsoft\\windows\\storage", NULL, NULL, 0, NULL, 0, 0, &m_pServiceDisk);
		if (FAILED(m_hr)) {
			PCINFO("Could not connect");
			m_pLocator->Release();
			CoUninitialize();
			return;
		}

		m_hr = CoSetProxyBlanket(m_pService, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
		if (FAILED(m_hr)) {
			PCINFO("Could not set proxy blanket");
			m_pService->Release();
			m_pLocator->Release();
			CoUninitialize();
			return;
		}
	}

	std::ostream& operator<<(std::ostream& os, const PCInfo& pcInfo) {
		os << "CPU Name: " << pcInfo.m_CPUName << std::endl;
		os << "Cores: " << pcInfo.m_Cores << std::endl;
		os << "Thread Count: " << pcInfo.m_ThreadCount << std::endl;
		os << "Base Clock Speed: " << pcInfo.m_BaseClockSpeed << std::endl;
		os << "Motherboard: " << pcInfo.m_Mob << std::endl;
		os << "System Name: " << pcInfo.m_Systemname << std::endl;
		os << "Memory Capacity: " << pcInfo.m_MemoryCapacity << std::endl;
		os << "Memory Speed: " << pcInfo.m_MemorySpeed << std::endl;
		os << "Memory Type: " << pcInfo.m_MemoryType << std::endl;
		os << "Memory Dimms: " << pcInfo.m_MemoryDimms << std::endl;
		os << "MAC Addresses: ";

		for (auto& mac : pcInfo.m_MACAddres) {
			os << mac << ", ";
		}
		os << std::endl;

		os << "Storage Names: ";
		for (auto& storage : pcInfo.m_StorageNames) {
			os << storage << ", ";
		}
		os << std::endl;

		os << "Disk Sizes: ";
		for (auto& size : pcInfo.m_DiskSize) {
			os << size << ", ";
		}
		os << std::endl;

		os << "Disk Types: ";
		for (auto& type : pcInfo.m_DiskType) {
			os << type << ", ";
		}
		os << std::endl;

		os << "Video Names: ";
		for (auto& video : pcInfo.m_VideoName) {
			os << video << ", ";
		}
		os << std::endl;

		os << "Vram: ";
		for (auto& vram : pcInfo.m_Vram) {
			os << vram << ", ";
		}
		os << std::endl;
		return os;
		
	}
}