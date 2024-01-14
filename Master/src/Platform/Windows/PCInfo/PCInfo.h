#pragma once

#include <WbemCli.h>
#pragma comment(lib, "wbemuuid.lib")


#include <windows.h>
#include <sysinfoapi.h>

#include <list>





namespace Puppeteer
{
    class PCInfo
    {
    public:
        std::string             m_CPUName;
        int                     m_Cores;
        int                     m_ThreadCount;
        int                     m_BaseClockSpeed;

        std::string             m_Mob;

        std::string             m_Systemname;

        UINT64                  m_MemoryCapacity;
        int                     m_MemorySpeed;
        std::string             m_MemoryType;
        int                     m_MemoryDimms;


        std::list<std::string>  m_MACAddres;

        std::list<std::string>  m_StorageNames;
        std::list<UINT64>       m_DiskSize;
        std::list<std::string>  m_DiskType;

        std::list<std::string>  m_VideoName;
        std::list<UINT32>       m_Vram;


        PCInfo();
        ~PCInfo() = default;

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
                };

        #pragma pack(pop)
        std::string getMemoryType(BYTE b);
        std::string BstrToStdString(BSTR bstr);
        std::string getMediaType(int i);
        MemoryInformation* getMemoryInformation();
    };
}



