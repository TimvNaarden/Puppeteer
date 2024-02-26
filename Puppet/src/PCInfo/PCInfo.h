#pragma once

#include "Json/WriteJson.h"
#include "Json/ParseJson.h"

#include <WbemCli.h>
#pragma comment(lib, "wbemuuid.lib")

#include <windows.h>
#include <sysinfoapi.h>
#include <combaseapi.h>
#include <map>

#define PCINFO(x) std::cout << "PCInfo: " << x << std::endl;

namespace Puppeteer
{
    class PCInfo
    {
    public:
        std::string m_CPUName;
        int m_Cores;
        int m_ThreadCount;
        int m_BaseClockSpeed;

        std::string m_Mob;

        std::string m_Systemname;

        UINT64 m_MemoryCapacity;
        int m_MemorySpeed;
        std::string m_MemoryType;
        int m_MemoryDimms;


        std::vector<std::string> m_MACAddres;

        std::vector<std::string> m_StorageNames;
        std::vector<UINT64> m_DiskSize;
        std::vector<std::string> m_DiskType;

        std::vector<std::string> m_VideoName;
        std::vector<UINT32> m_Vram;
        
        PCInfo();
        PCInfo(bool GetLocal);
        PCInfo(std::map<std::string, std::string> map);
        ~PCInfo();


        std::map<std::string, std::string> toMap();
        void renew();

        friend std::ostream& operator<<(std::ostream& os, const PCInfo& pcInfo);

    private:
        #pragma pack(push) 
        #pragma pack(1)

        struct RawSMBIOSData
        {
            BYTE    Used20CallingMethod;
            BYTE    SMBIOSMajorVersion;
            BYTE    SMBIOSMinorVersion;
            BYTE    DmiRevision;
            DWORD   Length;
            BYTE    SMBIOSTableData[];
        };

        struct SMBIOSHEADER
        {
            BYTE type;
            BYTE length;
            WORD handle;
        };

        struct MemoryInformation {
            SMBIOSHEADER header;
            WORD physicalArrayHandle;
            WORD errorInformationHandle;
            WORD totalWidth;
            WORD dataWidth;
            WORD size;
            BYTE formFactor;
            BYTE deviceSet;
            BYTE deviceLocator;
            BYTE bankLocator;
            BYTE memoryType;
            WORD typeDetail;
            WORD speed;
            BYTE manufacturer;
            BYTE serialNumber;
            BYTE assetTag;
            BYTE partNumber;
            BYTE attributes;
            DWORD extendedSize;
        };

        #pragma pack(pop)
        std::string getMemoryType(BYTE b);
        std::string BstrToStdString(BSTR bstr);
        std::string getMediaType(int i);
        std::vector<MemoryInformation*> getMemoryInformation();
        void init();

        HRESULT	m_hr;

    };
}



