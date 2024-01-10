#pragma once
#include <WbemCli.h>
#include <list>
#pragma comment(lib, "wbemuuid.lib")

std::string getMemoryType(int input);
std::string getMediaType(int input);
std::string BstrToStdString(BSTR bstr);

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

    };
}


