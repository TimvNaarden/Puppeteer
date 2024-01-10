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


        // Get the cpu info using wmi
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
			// get the processor 
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
			// get the memory
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

				m_hr = clsObject->Get(L"MemoryType", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) 
				{
					std::cout << "Failed to get memory type" << std::endl;
					VariantClear(&vtProp);
				}
				else
				{
					m_MemoryType = getMemoryType(vtProp.intVal);
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
			// get the memory
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
			// get the video
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
			// get the network
			while((int)(m_hr = pEnumerator->Next(WBEM_INFINITE, 1, &clsObject, &returned)) == (int)WBEM_S_NO_ERROR)
			{
				m_hr = clsObject->Get(L"MACAddress", 0, &vtProp, 0, 0);
				if (FAILED(m_hr)) 
				{
					std::cout << "Failed to get mac address" << std::endl;
					VariantClear(&vtProp);
				} 
				else {
					m_MACAddres.push_back(BstrToStdString(vtProp.bstrVal));
					VariantClear(&vtProp);
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


				m_hr = clsObject->Get(L"DeviceId", 0, &vtProp, 0, 0);
				if (FAILED(m_hr))
				{
					std::cout << "Failed to get storage name" << std::endl;
					VariantClear(&vtProp);
				}
				else 
				{
					m_StorageNames.push_back(getMediaType(vtProp.intVal));
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
					switch (vtProp.intVal) {
						case 0:
							m_DiskType.push_back("Unspecified");
							break;
						case 3:
							m_DiskType.push_back("HDD");
							break;
						case 4:
							m_DiskType.push_back("SSD");
							break;
						case 5:
							m_DiskType.push_back("SCM");
							break;
						default:
							m_DiskType.push_back("Unknown");
							break;
					}	
					VariantClear(&vtProp);
				}
				clsObject->Release();		
			}
			pEnumerator->Release();
		}

	}
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


std::string getMediaType(int input)  
{
	switch(input) {
		case 0:
			return "Unknown";

		case 3:
			return "HDD";

		case 4:
			return "SSD";

		case 5:
			return "SCM";

		default:
			return "Unknown";			
	}
}

std::string getMemoryType(int input) 
{
	switch(input) {
		case 0:
			return "Unknown";

		case 1:
			return "Other";

		case 2:
			return "DRAM";

		case 3:
			return "Synchronous DRAM";

		case 4:
			return "Cache DRAM";

		case 5:
			return "EDO";

		case 6:
			return "EDRAM";

		case 7:
			return "VRAM";

		case 8:
			return "SRAM";

		case 9:
			return "RAM";

		case 10:
			return "ROM";

		case 11:
			return "Flash";

		case 12:
			return "EEPROM";

		case 13:
			return "FEPROM";

		case 14:
			return "EPROM";

		case 15:
			return "CDRAM";

		case 16:
			return "3DRAM";

		case 17:
			return "SDRAM";

		case 18:
			return "SGRAM";

		case 19:
			return "RDRAM";

		case 20:
			return "DDR";

		case 21:
			return "DDR2";

		case 22:	
			return "DDR2 FB-DIMM";

		case 24:
			return "DDR3";

		case 25:
			return "FBD2";

		case 26:
			return "DDR4";

		default:
			return "Unknown";			
	}
}

