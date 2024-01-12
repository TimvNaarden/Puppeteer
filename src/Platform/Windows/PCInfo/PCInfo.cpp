#include "pch.h"
#include "PCInfo.h"



namespace Puppeteer 
{
	HRESULT	m_hr = S_OK;
	PCInfo::PCInfo()
	{
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

		m_hr = CoInitializeEx(0, COINIT_MULTITHREADED);
		if (FAILED(m_hr)) 
		{
			std::cout << "Failed to initialize COM library" << std::endl;
			return;
		}

		m_hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0);
		if (FAILED(m_hr)) 
		{
			std::cout << "Failed to initialize security" << std::endl;
			CoUninitialize();
			return;
		}

		IWbemLocator* pLocator = NULL;
		m_hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLocator);
		if(FAILED(m_hr))
		{
			std::cout << "Failed to create IWbemLocator object" << std::endl;
			CoUninitialize();
			return;
		}

		IWbemServices* pService = NULL;
		m_hr = pLocator->ConnectServer(L"root\\CIMV2", NULL, NULL, 0, NULL, 0, 0, &pService);
		if (FAILED(m_hr)) 
		{
			std::cout << "Could not connect" << std::endl;
			pLocator->Release();
			CoUninitialize();
			return;
		}

		IWbemServices* pServiceDisk = NULL;
		m_hr = pLocator->ConnectServer(L"root\\microsoft\\windows\\storage", NULL, NULL, 0, NULL, 0, 0, &pServiceDisk);
		if (FAILED(m_hr))
		{
			std::cout << "Could not connect" << std::endl;
			pLocator->Release();
			CoUninitialize();
			return;
		}

		m_hr = CoSetProxyBlanket(pService, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
		if (FAILED(m_hr))
		{
			std::cout << "Could not set proxy blanket" << std::endl;
			pService->Release();
			pLocator->Release();
			CoUninitialize();
			return;
		}


		VARIANT vtProp = {};
		IEnumWbemClassObject* pEnumerator = NULL;
		IWbemClassObject* clsObject = 0;
		ULONG returned = 0;

		m_hr = pService->ExecQuery(L"WQL", L"SELECT * FROM Win32_Processor", WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
		if(FAILED(m_hr)) 
		{
			std::cout << "Query for processor failed" << std::endl;
		}
		else 
		{			
			m_hr = pEnumerator->Next(0, 1, &clsObject, &returned);
			if (FAILED(m_hr)) 
			{ 
				std::cout << m_hr << std::endl;
				std::cout << "Failed iteration" << std::endl;
				clsObject->Release();
				pEnumerator->Release();
			} 
			else 
			{
				m_hr = clsObject->Get(L"Name", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) 
				{
					std::cout << "Failed to get processor name" << std::endl;
					VariantClear(&vtProp);
				}
				else
				{
					m_CPUName = BstrToStdString(vtProp.bstrVal);
					VariantClear(&vtProp);
				}

				m_hr = clsObject->Get(L"SystemName", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) 
				{
					std::cout << "Failed to get system name" << std::endl;
					VariantClear(&vtProp);
				}
				else
				{
					m_Systemname = BstrToStdString(vtProp.bstrVal);
					VariantClear(&vtProp);
				}

				m_hr = clsObject->Get(L"ThreadCount", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) 
				{
					std::cout << "Failed to get amount of threads" << std::endl;
					VariantClear(&vtProp);
				}
				else
				{
					m_ThreadCount = vtProp.intVal;
					VariantClear(&vtProp);
				}


				m_hr = clsObject->Get(L"NumberOfCores", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) 
				{
					std::cout << "Failed to get number of cores" << std::endl;
					VariantClear(&vtProp);
				}
				else 
				{
					m_Cores = vtProp.intVal;
					VariantClear(&vtProp);
				}

				m_hr = clsObject->Get(L"MaxClockSpeed", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) 
				{
					std::cout << "Failed to get max clock speed" << std::endl;
					VariantClear(&vtProp);
				}
				else 
				{
					m_BaseClockSpeed = vtProp.intVal;
					VariantClear(&vtProp);
				}

				clsObject->Release();
				pEnumerator->Release();
			}	
		}

		m_hr = pService->ExecQuery(L"WQL", L"SELECT * FROM Win32_PhysicalMemory", WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
		if(FAILED(m_hr)) 
		{
			std::cout << "Query for memory failed" << std::endl;
			pEnumerator->Release();
		}
		else 
		{			
			m_hr = pEnumerator->Next(WBEM_INFINITE, 1, &clsObject, &returned);
			if (FAILED(m_hr)) 
			{ 
				std::cout << "Failed iteration" << std::endl;
				clsObject->Release();
				pEnumerator->Release();
			} 
			else 
			{		
				m_hr = clsObject->Get(L"Speed", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) 
				{
					std::cout << "Failed to get memory speed" << std::endl;
					VariantClear(&vtProp);
				}
				else
				{
					m_MemorySpeed = vtProp.intVal;
					VariantClear(&vtProp);
				}

				m_hr = clsObject->Get(L"capacity", 0, &vtProp, 0, 0);
				if (FAILED(m_hr))
				{
					std::cout << "Failed to get memory capacity" << std::endl;
					VariantClear(&vtProp);
				}
				else
				{
					m_MemoryCapacity += vtProp.ullVal;
					VariantClear(&vtProp);
				}

				m_MemoryDimms += 1;
				while ((int)(m_hr = pEnumerator->Next(WBEM_INFINITE, 1, &clsObject, &returned)) == (int)WBEM_S_NO_ERROR)
				{
					m_MemoryDimms += 1;
					m_hr = clsObject->Get(L"capacity", 0, &vtProp, 0, 0);
					if (FAILED(m_hr))
					{
						std::cout << "Failed to get memory capacity" << std::endl;
						VariantClear(&vtProp);
					}
					else
					{
						m_MemoryCapacity += vtProp.ullVal;
						VariantClear(&vtProp);
					}
					
				}

				clsObject->Release();
				pEnumerator->Release();

			}
		}

		m_hr = pService->ExecQuery(L"WQL", L"SELECT * FROM Win32_BaseBoard", WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
		if(FAILED(m_hr)) 
		{
			std::cout << "Query for motherboard failed" << std::endl;
		}
		else 
		{			
			m_hr = pEnumerator->Next(WBEM_INFINITE, 1, &clsObject, &returned);
			if (FAILED(m_hr)) 
			{ 
				std::cout << "Failed iteration" << std::endl;
				clsObject->Release();
				pEnumerator->Release();
			} 
			else 
			{
				m_hr = clsObject->Get(L"Manufacturer", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) 
				{
					std::cout << "Failed to get motherboard manufacturer" << std::endl;
					VariantClear(&vtProp);
				}
				else 
				{
					m_Mob = BstrToStdString(vtProp.bstrVal);
					VariantClear(&vtProp);
					m_Mob += " ";
				}

				m_hr = clsObject->Get(L"Product", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) 
				{
					std::cout << "Failed to get motherboard name" << std::endl;
					VariantClear(&vtProp);
				}
				else
				{
					m_Mob += BstrToStdString(vtProp.bstrVal);
					VariantClear(&vtProp);
				}

				clsObject->Release();
				pEnumerator->Release();
			}
		}

		m_hr = pService->ExecQuery(L"WQL", L"SELECT * FROM Win32_VideoController", WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
		if(FAILED(m_hr)) 
		{
			std::cout << "Query for video failed" << std::endl;
		}
		else 
		{
			while((int)(m_hr = pEnumerator->Next(WBEM_INFINITE, 1, &clsObject, &returned)) == (int)WBEM_S_NO_ERROR)
			{
				m_hr = clsObject->Get(L"Name", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) 
				{
					std::cout << "Failed to get video name" << std::endl;
					VariantClear(&vtProp);
				}
				else
				{
					m_VideoName.push_back(BstrToStdString(vtProp.bstrVal));
					VariantClear(&vtProp);
				}

				m_hr = clsObject->Get(L"AdapterRAM", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) 
				{
					std::cout << "Failed to get video ram" << std::endl;
					VariantClear(&vtProp);
				}
				else
				{
					m_Vram.push_back(vtProp.lVal);
					VariantClear(&vtProp);
				}

			}
			clsObject->Release();
			pEnumerator->Release();

		}
		
		m_hr = pService->ExecQuery(L"WQL", L"SELECT * FROM Win32_NetworkAdapterConfiguration", WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
		if(FAILED(m_hr))
		{
			std::cout << "Query for network failed" << std::endl;
			pEnumerator->Release();
		}
		else
		{
			while((int)(m_hr = pEnumerator->Next(WBEM_INFINITE, 1, &clsObject, &returned)) == (int)WBEM_S_NO_ERROR)
			{
				m_hr = clsObject->Get(L"MACAddress", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) 
				{
					std::cout << "Failed to get mac address" << std::endl;
					VariantClear(&vtProp);
				} 
				else {
					if (vtProp.bstrVal != L"")
					{
						m_MACAddres.push_back(BstrToStdString(vtProp.bstrVal));
						VariantClear(&vtProp);
					}
				}
							
				clsObject->Release();	
			}
			pEnumerator->Release();
		}

		m_hr = pServiceDisk->ExecQuery(L"WQL", L"SELECT * FROM MSFT_PhysicalDisk", WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
		if (FAILED(m_hr))
		{
			std::cout << "Failed to open namespace" << std::endl;
			
		}
		else {
			while ((int)(m_hr = pEnumerator->Next(WBEM_INFINITE, 1, &clsObject, &returned)) == (int)WBEM_S_NO_ERROR)
			{
				m_hr = clsObject->Get(L"Size", 0, &vtProp, 0, 0);
				if (FAILED(m_hr))
				{
					std::cout << "Failed to get storage size" << std::endl;
					VariantClear(&vtProp);
				}
				else
				{
					m_DiskSize.push_back(vtProp.ullVal);
					VariantClear(&vtProp);
				}

				m_hr = clsObject->Get(L"FriendlyName", 0, &vtProp, 0, 0);
				if (FAILED(m_hr))
				{
					std::cout << "Failed to get storage name" << std::endl;
					VariantClear(&vtProp);
				}
				else 
				{
					m_StorageNames.push_back(BstrToStdString(vtProp.bstrVal));
					VariantClear(&vtProp);
				}

				m_hr = clsObject->Get(L"MediaType", 0, &vtProp, 0, 0);
				if (FAILED(m_hr))
				{
					std::cout << "Failed to get storage type" << std::endl;
					VariantClear(&vtProp);
				}
				else
				{
					m_DiskType.push_back(getMediaType(vtProp.intVal));
					VariantClear(&vtProp);
				}
				clsObject->Release();		
			}
			pEnumerator->Release();
		}

		// Cleanup
		pService->Release();
		pLocator->Release();
		CoUninitialize();

		MemoryInformation* memoryInformation = getMemoryInformation();
		m_MemoryType = getMemoryType(memoryInformation->memoryType);
	}
}

MemoryInformation* getMemoryInformation()
{
	RawSMBIOSData* smBiosData = nullptr;

	DWORD smBiosDataSize = GetSystemFirmwareTable('RSMB', 0, NULL, 0);

	if (smBiosDataSize == 0) 
	{
		std::cerr << "Failed to get SMBIOS data size." << std::endl;
		return nullptr;
	}

	smBiosData = (RawSMBIOSData*)HeapAlloc(GetProcessHeap(), 0, smBiosDataSize);

	DWORD bytesRetrieved = GetSystemFirmwareTable('RSMB',0, smBiosData, smBiosDataSize);

	if (bytesRetrieved != smBiosDataSize) 
	{
		std::cerr << "Failed to get SMBIOS data." << std::endl;
		HeapFree(GetProcessHeap(), 0, smBiosData);
		return nullptr;
	}

	BYTE* data = smBiosData->SMBIOSTableData;

	while (data < smBiosData->SMBIOSTableData + smBiosData->Length) 
	{
		BYTE* next;
		SMBIOSHEADER* header = (SMBIOSHEADER*)data;

		if (header->length < 4)
		{
			break;
		}

		if (header->type == 0x11 && header->length >= 0x19 ) 
		{
			return (MemoryInformation*)header;
		}

		next = data + header->length;

		while (next < smBiosData->SMBIOSTableData + smBiosData->Length && (next[0] != 0 || next[1] != 0)) {
			next++;
		}
		next += 2;

		data = next;

	}

	std::cerr << "Failed to get SMBIOS data." << std::endl; 
	HeapFree(GetProcessHeap(), 0, smBiosData);
	return nullptr;
}
std::string BstrToStdString(BSTR bstr) 
{
	if (!bstr) return "";

	int len = SysStringLen(bstr);
	std::wstring wideStr(bstr, len);

	int size_needed = WideCharToMultiByte(CP_ACP, 0, &wideStr[0], len, nullptr, 0, nullptr, nullptr);
	std::string result(size_needed, 0);
	WideCharToMultiByte(CP_ACP, 0, &wideStr[0], len, &result[0], size_needed, nullptr, nullptr);

	return result;
}

std::string getMemoryType(BYTE b)
{
	switch (b)
	{
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

std::string getMediaType(int i) 
{
	switch (i) 
	{
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
