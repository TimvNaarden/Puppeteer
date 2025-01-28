#include <curl/curl.h>
#include <iostream>
#include <Json/ParseJson.h>
#include <map>
#include <string>

#include <fstream>
#include <strsafe.h>
#include <urlmon.h>
#include <Windows.h>
#include <thread>
#include <chrono>
#pragma comment(lib, "urlmon.lib")

#include <crtdbg.h>
#include <netfw.h>
#include <objbase.h>
#include <oleauto.h>
#pragma comment( lib, "ole32.lib" )
#pragma comment( lib, "oleaut32.lib" )

#include <WtsApi32.h>
#pragma comment(lib, "Wtsapi32.lib")

constexpr auto SERVICE_NAME = L"Puppet Service";

struct Version {
    int Major;
    int Minor;
    int Revision;
};

static std::string version = "V0.0.0";

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;
HANDLE                hPuppetProcess = NULL;
DWORD 			      dwActiveSessionId = 0;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
LPHANDLER_FUNCTION_EX WINAPI ServiceCtrlHandler(DWORD, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
DWORD GetPIDInSession(DWORD dwSessionId, LPWSTR pIdentifier);
static int StartPuppetService();
BOOL ServiceExists(const TCHAR* serviceName);

void GetPuppetVersion();
static Version ParseVersion(std::string version);
static bool CompareVersions(std::string version1, std::string version2);
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output);
void UpdatePuppet();

static int InstallService();
static int RunNormal();

static HRESULT WindowsFirewallInitialize(OUT INetFwProfile** fwProfile);
static int WindowsFirewallIsOn(IN INetFwProfile* fwProfile);
static int WindowsFirewallAppIsEnabled(IN INetFwProfile* fwProfile, IN const wchar_t* fwProcessImageFileName);
static HRESULT WindowsFirewallAddApp(IN INetFwProfile* fwProfile, IN const wchar_t* fwProcessImageFileName, IN const wchar_t* fwName);
static int AddToFirewall();

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app };
    DWORD Status = E_FAIL;

    g_StatusHandle = RegisterServiceCtrlHandlerExW(SERVICE_NAME, (LPHANDLER_FUNCTION_EX) ServiceCtrlHandler, NULL);

    if (g_StatusHandle == NULL) return;

    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
        OutputDebugString(L"Puppet Service: ServiceMain: SetServiceStatus returned error");
        logFile << "Puppet Service: ServiceMain: SetServiceStatus returned error" << std::endl;
    }
    
     // Create a service stop event to wait on later
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        // Error creating event
        // Tell service controller we are stopped and exit
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
            OutputDebugString(L"Puppet Service: ServiceMain: SetServiceStatus returned error");
            logFile << "Puppet Service: ServiceMain: SetServiceStatus returned error" << std::endl;
        }

        logFile << std::endl;
        logFile.close();
        return;
    }

    // Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)  {
        OutputDebugString(L"Puppet Service: ServiceMain: SetServiceStatus returned error");
        logFile << "Puppet Service: ServiceMain: SetServiceStatus returned error" << std::endl;
    }

    GetPuppetVersion();
    UpdatePuppet();

    if (!AddToFirewall()) {
        logFile << "Failed to add to firewall" << std::endl;
        std::cerr << "Failed to add to firewall" << std::endl;
    }
    else {
        logFile << "Added to firewall" << std::endl;
        std::cout << "Added to firewall" << std::endl;
    }

    while (WTSGetActiveConsoleSessionId() == 0 || WTSGetActiveConsoleSessionId() == 0xFFFFFFFF) continue;
    
    dwActiveSessionId = WTSGetActiveConsoleSessionId();
    if (!StartPuppetService()) {
        WaitForSingleObject(g_ServiceStopEvent, INFINITE);
    }

    CloseHandle(g_ServiceStopEvent);

    // Tell the service controller we are stopped
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
        OutputDebugString(L"Puppet Service: ServiceMain: SetServiceStatus returned error");
        logFile << "Puppet Service: ServiceMain: SetServiceStatus returned error" << std::endl;
    }

    logFile << std::endl;
    logFile.close();
    return;
}

LPHANDLER_FUNCTION_EX WINAPI ServiceCtrlHandler(DWORD CtrlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
    std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app };
    logFile << "ServiceCtrlHandler" << std::endl;
    switch (CtrlCode) {
        case SERVICE_CONTROL_STOP: [[fallthrough]];
        case SERVICE_CONTROL_SHUTDOWN:
            if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING) break;

            g_ServiceStatus.dwControlsAccepted = 0;
            g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            g_ServiceStatus.dwWin32ExitCode = 0;
            g_ServiceStatus.dwCheckPoint = 4;

            if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
                OutputDebugString(L"Puppet Service: ServiceCtrlHandler: SetServiceStatus returned error");
                logFile << "Puppet Service: ServiceCtrlHandler: SetServiceStatus returned error" << std::endl;
            }
            // This will signal the worker thread to start shutting down
            SetEvent(g_ServiceStopEvent);
            hPuppetProcess = OpenProcess(PROCESS_TERMINATE, FALSE, GetPIDInSession(dwActiveSessionId, L"Puppet.exe"));
            if (hPuppetProcess != NULL) {
				TerminateProcess(hPuppetProcess, 0);
				CloseHandle(hPuppetProcess);
			}
            dwActiveSessionId = WTSGetActiveConsoleSessionId();
            logFile << "Service stopped" << std::endl;
            
            logFile << std::endl;
            logFile.close();
            break;
        default:
            break;
    }
    if(dwEventType == SERVICE_CONTROL_SESSIONCHANGE) {
        if (lpEventData == NULL) {
            logFile.close();
            return NO_ERROR;
        }

		WTSSESSION_NOTIFICATION* pSessionNotification = (WTSSESSION_NOTIFICATION*)lpEventData;
        if (pSessionNotification->dwSessionId == 0xFFFFFFFF) {
            logFile.close();
            return NO_ERROR;
        }

        hPuppetProcess = OpenProcess(PROCESS_TERMINATE, FALSE, GetPIDInSession(WTSGetActiveConsoleSessionId(), L"Puppet.exe"));
        if (hPuppetProcess != NULL) {
            TerminateProcess(hPuppetProcess, 0);
            CloseHandle(hPuppetProcess);
        }
                
		logFile << "Session Changed" << std::endl;
        logFile << "Session ID: " << pSessionNotification->dwSessionId << std::endl;
        if (!StartPuppetService()) {
            logFile.close();
            return NO_ERROR;
        }

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING) {
            logFile.close();
            return NO_ERROR;
        }

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
            OutputDebugString(L"Puppet Service: ServiceCtrlHandler: SetServiceStatus returned error");
            logFile << "Puppet Service: ServiceCtrlHandler: SetServiceStatus returned error" << std::endl;
        }
        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);

        logFile << "Service stopped" << std::endl;

        logFile << std::endl;
        logFile.close();
        return NO_ERROR;     
    }
    return NO_ERROR;
}

DWORD GetPIDInSession(DWORD dwSessionId, LPWSTR pIdentifier) {
    DWORD dwWinlogonPID = 0;
    PWTS_PROCESS_INFO pProcessInfo = NULL;
    DWORD dwProcessCount = 0;

    if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &dwProcessCount)) {
        for (DWORD i = 0; i < dwProcessCount; ++i) {
            if (_wcsicmp(pProcessInfo[i].pProcessName, pIdentifier) == 0 && pProcessInfo[i].SessionId == dwSessionId) {
                dwWinlogonPID = pProcessInfo[i].ProcessId;
                break;
            }
        }
        WTSFreeMemory(pProcessInfo);
    }
    else {
        std::cerr << "WTSEnumerateProcesses failed. Error: " << GetLastError() << std::endl;
    }

    return dwWinlogonPID;
}

static int StartPuppetService() {
    std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app };
    const wchar_t* exePath = L"Puppet.exe";
    DWORD winlogonPID = GetPIDInSession(WTSGetActiveConsoleSessionId(), L"winlogon.exe");
    if (winlogonPID == 0) {
        logFile << "Failed to get winlogon.exe PID in session "<< std::endl;
        std::cerr << "Failed to get winlogon.exe PID in session "<< std::endl;

        logFile.close();
        return 1;
    }

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, winlogonPID);
    if (hProcess == NULL) {
        logFile << "Failed to open winlogon.exe process. Error: " << GetLastError() << std::endl;
        std::cerr << "Failed to open winlogon.exe process. Error: " << GetLastError() << std::endl;
        
        logFile.close();
        return 1;
    }

    HANDLE hToken = NULL;
    if (!OpenProcessToken(hProcess, TOKEN_DUPLICATE, &hToken)) {
        logFile << "Failed to open token of winlogon.exe. Error: " << GetLastError() << std::endl;
        std::cerr << "Failed to open token of winlogon.exe. Error: " << GetLastError() << std::endl;
        
        CloseHandle(hProcess);
        
        logFile.close();
        return 1;
    }

    HANDLE hPrimaryToken = NULL;
    if (!DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hPrimaryToken)) {
        logFile << "Failed to duplicate token. Error: " << GetLastError() << std::endl;
        std::cerr << "Failed to duplicate token. Error: " << GetLastError() << std::endl;
        
        CloseHandle(hToken);
        CloseHandle(hProcess);
        
        logFile.close();
        return 1;
    }

    CloseHandle(hToken);
    CloseHandle(hProcess);

    STARTUPINFO si = { sizeof(si) };
    si.lpDesktop = L"winsta0\\default"; // Ensure it runs on the interactive desktop
    PROCESS_INFORMATION pi;
    
    if (!CreateProcessAsUser(
        hPrimaryToken,  // User token   
        exePath,    // Path to the executable
        NULL,       // Command line
        NULL,       // Process attributes
        NULL,       // Thread attributes
        FALSE,      // Inherit handles
        CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_PROCESS_GROUP | CREATE_BREAKAWAY_FROM_JOB,      // Creation flags
        NULL,       // Environment
        NULL,       // Current directory
        &si,        // Startup info
        &pi         // Process information
    )) {
        logFile << "Failed to create process: " << GetLastError() << std::endl;
        std::cerr << "Failed to create process: " << GetLastError() << std::endl;

        CloseHandle(hPrimaryToken);

        logFile.close();
        return 1;
    }
    else {
        logFile << "Process created." << std::endl;
        std::cout << "Process created." << std::endl;

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hPrimaryToken);
    }

    logFile.close();
    return 0;
}

BOOL ServiceExists(const TCHAR* serviceName) {
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) {
        std::cerr << "Failed to open Service Control Manager" << std::endl;
        return FALSE;
    }

    SC_HANDLE service = OpenService(scm, serviceName, SERVICE_QUERY_CONFIG);
    if (!service) {
        CloseServiceHandle(scm);
        return FALSE;
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return TRUE;
}

static void GetPuppetVersion() {
    std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app };

    FILE* file;
    file = _popen("Puppet.exe -v", "r");
    if (file == NULL) {
		std::cerr << "Failed to open Puppet.exe"<< std::endl;
        logFile << "Failed to open Puppet.exe" << std::endl;

        logFile.close();

        return;
	}
    char buffer[128];
	std::string result = "";
    while (fgets(buffer, 128, file) != NULL) {
		result += buffer;
	}
	_pclose(file);
    if (!result._Starts_with("V")) {
        logFile << "Failed to get version" << std::endl;
        std::cerr << "Failed to get version" << std::endl;

        logFile.close();
        return;
    }
    version = result;
}

static Version ParseVersion(std::string version) {
    Version Result = {};
    std::string line;
    std::stringstream ss(version);
    int i = 0;
    while (std::getline(ss, line, '.')) {
        if (i == 0) Result.Major = std::stoi(line);
        if (i == 1) Result.Minor = std::stoi(line);
        if (i == 2) Result.Revision = std::stoi(line);
        i++;
    }
    return Result;
}

static bool CompareVersions(std::string version1, std::string version2) {
    Version v1 = ParseVersion(version1);
    Version v2 = ParseVersion(version2);

    if(v1.Major > v2.Major) return true;
    if(v1.Major < v2.Major) return false;

    if(v1.Minor > v2.Minor) return true;
    if(v1.Minor < v2.Minor) return false;

    if(v1.Revision > v2.Revision) return true;
    if(v1.Revision < v2.Revision) return false;

    return false;
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

static void UpdatePuppet() {
    std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app };

    CURL* curl;
    CURLcode res;
    std::string config_data;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/TimvNaarden/Puppeteer/puppet-config/puppetconfig.json");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &config_data);


        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            logFile << "Failed to fetch config file: " << std::endl;
            std::cerr << "Failed to fetch config file: " << std::endl;
            curl_easy_cleanup(curl);

            logFile << std::endl;
            logFile.close();

            return;
        }
        curl_easy_cleanup(curl);
    }
    else {
        logFile << "Failed to initialize cURL." << std::endl;
        std::cerr << "Failed to initialize cURL." << std::endl;

        logFile << std::endl;
        logFile.close();
        return;
    }
    char* workdir = new char[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, workdir);
    logFile << "Working directory: " << workdir << std::endl;
    std::cout << "Working directory: " << workdir << std::endl;

    std::map<std::string, std::string> config = ParseJson<std::map<std::string, std::string>>(config_data);
    logFile << "Current version: " << version << std::endl;
    logFile << "New version: " << "V" << config["version"] << std::endl;
    std::cout << "Current version: " << version << std::endl;
    std::cout << "New version: " << "V" << config["version"] << std::endl;

    if (CompareVersions(config["version"], version.substr(1, version.size() -1))) {
        logFile << "New version Found" << std::endl;
        std::cout << "New version Found" << std::endl;

        HRESULT result = URLDownloadToFileA(NULL, config["url"].data(),std::string(std::string(workdir) + "\\temp.zip").data(), 0, NULL);
        if (FAILED(result)) {
			logFile << "Failed to download new version" << std::endl;
			std::cerr << "Failed to download new version" << std::endl;

			logFile << std::endl;
			logFile.close();
			return;
		}
        ShellExecuteA(NULL, "runas", "tar", "-xf temp.zip", NULL, SW_HIDE);
		std::this_thread::sleep_for(std::chrono::seconds(2));
        DeleteFileW(L"temp.zip");

        logFile << "Puppet updated" << std::endl;
        std::cout << "Puppet updated" << std::endl;

        logFile.close();
    }
}

static int InstallService() {
    std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app};

    logFile << "Creating service" << std::endl;
    std::cout << "Creating service" << std::endl;

    LPWSTR ExecutablePath = new WCHAR[MAX_PATH];
    GetModuleFileName(NULL, ExecutablePath, 260);

    if (!ServiceExists(SERVICE_NAME)) {
        SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
        if (!scm) {
            std::cerr << "Failed to open Service Control Manager" << std::endl;
            logFile << "Failed to open Service Control Manager" << std::endl;

            logFile << std::endl;
            logFile.close();

            return 1;
        }

        SC_HANDLE service = CreateService(
            scm,
            SERVICE_NAME,              // Service name
            SERVICE_NAME,              // Display name
            SERVICE_ALL_ACCESS,        // Desired access
            SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS, // Service type
            SERVICE_AUTO_START,        // Start type
            SERVICE_ERROR_NORMAL,      // Error control type
            ExecutablePath,            // Path to service binary
            NULL,                      // Load order group
            NULL,                      // Tag identifier
            NULL,                      // Dependencies
            NULL,                      // Service start name
            NULL                       // Password
        );

        if (!service) {
            CloseServiceHandle(scm);

            std::cerr << "Failed to create service: " << GetLastError() << std::endl;
            logFile << "Failed to create service: " << GetLastError() << std::endl;

            logFile << std::endl;
            logFile.close();
            return 1;
        }

        SERVICE_DESCRIPTION serviceDesc{};
        serviceDesc.lpDescription = (LPWSTR)L"Client Side Service For Puppeteer, https://github.com/timvnaarden/puppeteer";
        ChangeServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, &serviceDesc);

        CloseServiceHandle(service);
        CloseServiceHandle(scm);

        std::cout << "SService created" << std::endl;
        logFile << "Service created" << std::endl;
    }
    else {
        std::cout << "Service already exists!" << std::endl;
        logFile << "Service already exists" << std::endl;
    }

    logFile << std::endl;
    logFile.close();

    return 0;
}

static int RunNormal() {
	std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app };

	logFile << "Starting PuppetUpdate - Manual Exe" << std::endl;
	std::cout << "Starting PuppetUpdate - Manual Exe" << std::endl;

	GetPuppetVersion();
	UpdatePuppet();

    if (!AddToFirewall()) {
        logFile << "Failed to add to firewall" << std::endl;
        std::cerr << "Failed to add to firewall" << std::endl;
    }
    else {
        logFile << "Added to firewall" << std::endl;
        std::cout << "Added to firewall" << std::endl;
    }


	logFile << "Starting Puppet" << std::endl;
    std::cout << "Starting Puppet" << std::endl;
	
    ShellExecuteA(NULL, "runas", "Puppet.exe", NULL, NULL, SW_HIDE);

	logFile << std::endl;
	logFile.close();

	return 0;
}

static HRESULT WindowsFirewallInitialize(OUT INetFwProfile** fwProfile) {
    std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app };
    HRESULT hr = S_OK;
    INetFwMgr* fwMgr = NULL;
    INetFwPolicy* fwPolicy = NULL;
    *fwProfile = NULL;

    // Create an instance of the firewall settings manager.
    hr = CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&fwMgr);
    if (FAILED(hr)) {
        logFile << "CoCreateInstance failed" << std::endl;
        std::cerr << "CoCreateInstance failed" << std::endl;

        if (fwPolicy != NULL) fwPolicy->Release();
        if (fwMgr != NULL) fwMgr->Release();

        logFile.close();
        return S_FALSE;
    }

    // Retrieve the local firewall policy.
    hr = fwMgr->get_LocalPolicy(&fwPolicy);
    if (FAILED(hr)) {
        logFile << "get_LocalPolicy failed" << std::endl;
        std::cerr << "get_LocalPolicy failed" << std::endl;

        if (fwPolicy != NULL) fwPolicy->Release();
        if (fwMgr != NULL) fwMgr->Release();

        logFile.close();
        return S_FALSE;
    }

    hr = fwPolicy->get_CurrentProfile(fwProfile);
    if (FAILED(hr)) {
        logFile << "get_CurrentProfile failed" << std::endl;
        std::cerr << "get_CurrentProfile failed" << std::endl;

        if (fwPolicy != NULL) fwPolicy->Release();
        if (fwMgr != NULL) fwMgr->Release();

        logFile.close();
        return S_FALSE;
    }
    logFile.close();
    return hr;
}

static int WindowsFirewallIsOn(IN INetFwProfile* fwProfile) {
    std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app };
    HRESULT hr = S_OK;
    VARIANT_BOOL fwEnabled;

    hr = fwProfile->get_FirewallEnabled(&fwEnabled);
    if (FAILED(hr)) {
        logFile << "get_FirewallEnabled failed" << std::endl;
        std::cerr << "get_FirewallEnabled failed" << std::endl;

        logFile.close();
        return S_FALSE;
    }

    if (fwEnabled != VARIANT_FALSE) {
        logFile << "The firewall is on" << std::endl;
        std::cout << "The firewall is on" << std::endl;

        logFile.close();
        return 1;
    }

    logFile << "The firewall is off" << std::endl;
    std::cout << "The firewall is off" << std::endl;

    logFile.close();
    return 0;
}

static int WindowsFirewallAppIsEnabled(IN INetFwProfile* fwProfile, IN const wchar_t* fwProcessImageFileName) {
    std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app };
    HRESULT hr = S_OK;
    BSTR fwBstrProcessImageFileName = NULL;
    VARIANT_BOOL fwEnabled;
    INetFwAuthorizedApplication* fwApp = NULL;
    INetFwAuthorizedApplications* fwApps = NULL;

    hr = fwProfile->get_AuthorizedApplications(&fwApps);
    if (FAILED(hr)) {
        logFile << "get_AuthorizedApplications failed" << std::endl;
        std::cerr << "get_AuthorizedApplications failed" << std::endl;

        if (fwApp != NULL) fwApp->Release();
        if (fwApps != NULL) fwApps->Release();

        logFile.close();
        return 1;
    }

    fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);
    if (fwBstrProcessImageFileName == NULL) {
        logFile << "SysAllocString failed" << std::endl;
        std::cerr << "SysAllocString failed" << std::endl;

        SysFreeString(fwBstrProcessImageFileName);
        if (fwApp != NULL) fwApp->Release();
        if (fwApps != NULL) fwApps->Release();

        logFile.close();
        return 1;
    }

    hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp);
    if (FAILED(hr)) {
        //logFile << "getItem failed" << std::endl;
        //std::cerr << "getItem failed" << std::endl;

        SysFreeString(fwBstrProcessImageFileName);
        if (fwApp != NULL) fwApp->Release();
        if (fwApps != NULL) fwApps->Release();

        logFile.close();
        return 0;
    }

    hr = fwApp->get_Enabled(&fwEnabled);
    if (FAILED(hr)) {
        logFile << "get_Enabled failed" << std::endl;
        std::cerr << "get_Enabled failed" << std::endl;

        SysFreeString(fwBstrProcessImageFileName);
        if (fwApp != NULL) fwApp->Release();
        if (fwApps != NULL) fwApps->Release();

        logFile.close();
        return 1;
    }

    if (fwEnabled != VARIANT_FALSE) {
        logFile << "App already Enabled" << std::endl;
        std::cerr << "App already Enabled" << std::endl;

        SysFreeString(fwBstrProcessImageFileName);
        if (fwApp != NULL) fwApp->Release();
        if (fwApps != NULL) fwApps->Release();

        logFile.close();
        return 1;
    }
    logFile << "App is Disabled" << std::endl;
    std::cerr << "App is Disabled" << std::endl;

    SysFreeString(fwBstrProcessImageFileName);
    if (fwApp != NULL) fwApp->Release();
    if (fwApps != NULL) fwApps->Release();

    logFile.close();
    return 0;
}

static HRESULT WindowsFirewallAddApp(IN INetFwProfile* fwProfile, IN const wchar_t* fwProcessImageFileName, IN const wchar_t* fwName) {
    std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app };
    HRESULT hr = S_OK;
    BSTR fwBstrName = NULL;
    BSTR fwBstrProcessImageFileName = NULL;
    INetFwAuthorizedApplication* fwApp = NULL;
    INetFwAuthorizedApplications* fwApps = NULL;

    if (WindowsFirewallAppIsEnabled(fwProfile, fwProcessImageFileName))  return S_FALSE;

    hr = fwProfile->get_AuthorizedApplications(&fwApps);
    if (FAILED(hr)) {
        logFile << "get_AuthorizedApplications failed" << std::endl;
        std::cerr << "get_AuthorizedApplications failed" << std::endl;

        if (fwApp != NULL) fwApp->Release();
        if (fwApps != NULL) fwApps->Release();

        logFile.close();
        return hr;
    }

    hr = CoCreateInstance(__uuidof(NetFwAuthorizedApplication), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwAuthorizedApplication), (void**)&fwApp);
    if (FAILED(hr)) {
        logFile << "CoCreateInstance failed" << std::endl;
        std::cerr << "CoCreateInstance failed" << std::endl;

        if (fwApp != NULL) fwApp->Release();
        if (fwApps != NULL) fwApps->Release();

        logFile.close();
        return hr;
    }

    fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);
    if (fwBstrProcessImageFileName == NULL) {
        logFile << "SysAllocString failed" << std::endl;
        std::cerr << "SysAllocString failed" << std::endl;

        if (fwApp != NULL) fwApp->Release();
        if (fwApps != NULL) fwApps->Release();

        logFile.close();
        return E_OUTOFMEMORY;
    }

    hr = fwApp->put_ProcessImageFileName(fwBstrProcessImageFileName);
    if (FAILED(hr)) {
        logFile << "put_ProcessImageFileName failed" << std::endl;
        std::cerr << "put_ProcessImageFileName failed" << std::endl;

        if (fwApp != NULL) fwApp->Release();
        if (fwApps != NULL) fwApps->Release();
        SysFreeString(fwBstrProcessImageFileName);
        SysFreeString(fwBstrName);

        logFile.close();
        return hr;
    }

    fwBstrName = SysAllocString(fwName);
    if (SysStringLen(fwBstrName) == 0) {
        logFile << "SysAllocString failed" << std::endl;
        std::cerr << "SysAllocString failed" << std::endl;

        if (fwApp != NULL) fwApp->Release();
        if (fwApps != NULL) fwApps->Release();
        SysFreeString(fwBstrProcessImageFileName);
        SysFreeString(fwBstrName);

        logFile.close();
        return S_FALSE;
    }

    hr = fwApp->put_Name(fwBstrName);
    if (FAILED(hr)) {
        logFile << "put_Name failed" << std::endl;
        std::cerr << "put_Name failed" << std::endl;

        if (fwApp != NULL) fwApp->Release();
        if (fwApps != NULL) fwApps->Release();
        SysFreeString(fwBstrProcessImageFileName);
        SysFreeString(fwBstrName);

        logFile.close();
        return hr;
    }
    hr = fwApp->put_Enabled(VARIANT_TRUE);
    if (FAILED(hr)) {
		logFile << "put_Enabled failed" << std::endl;
		std::cerr << "put_Enabled failed" << std::endl;

		if (fwApp != NULL) fwApp->Release();
		if (fwApps != NULL) fwApps->Release();
		SysFreeString(fwBstrProcessImageFileName);
		SysFreeString(fwBstrName);

		logFile.close();
		return hr;
	}

    hr = fwApp->put_Scope(NET_FW_SCOPE_ALL);
    if (FAILED(hr)) {
		logFile << "put_Scope failed" << std::endl;
		std::cerr << "put_Scope failed" << std::endl;

		if (fwApp != NULL) fwApp->Release();
		if (fwApps != NULL) fwApps->Release();
		SysFreeString(fwBstrProcessImageFileName);
		SysFreeString(fwBstrName);

		logFile.close();
		return hr;
	}

    hr = fwApp->put_IpVersion(NET_FW_IP_VERSION_ANY);
    if (FAILED(hr)) {
        logFile << "put_IpVersion failed" << std::endl;
        std::cerr << "put_IpVersion failed" << std::endl;

        if (fwApp != NULL) fwApp->Release();
        if (fwApps != NULL) fwApps->Release();
        SysFreeString(fwBstrProcessImageFileName);
        SysFreeString(fwBstrName);

        logFile.close();
        return hr;
    }

    hr = fwApps->Add(fwApp);
    if (FAILED(hr)) {
        logFile << "Add failed" << std::endl;
        std::cerr << "Add failed" << std::endl;

        if (fwApp != NULL) fwApp->Release();
        if (fwApps != NULL) fwApps->Release();
        SysFreeString(fwBstrProcessImageFileName);
        SysFreeString(fwBstrName);

        logFile.close();
        return hr;
    }

    logFile << "Authorized application added" << std::endl;
    std::cout << "Authorized application added" << std::endl;

    fwProfile->put_FirewallEnabled(VARIANT_TRUE);
    SysFreeString(fwBstrName);
    SysFreeString(fwBstrProcessImageFileName);

    if (fwApp != NULL) fwApp->Release();
    if (fwApps != NULL) fwApps->Release();

    logFile.close();
    return S_OK;
}

static int AddToFirewall() {
    std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app };
    LPWSTR pathPuppet = new WCHAR[MAX_PATH];
    GetModuleFileName(NULL, pathPuppet, MAX_PATH);

    LPWSTR pathPuppetUpdate = wcsstr(pathPuppet, L"PuppetUpdate.exe");
    if (pathPuppet == pathPuppetUpdate) {
        logFile << "Failed to get path" << std::endl;
        std::cerr << "Failed to get path" << std::endl;

        delete[] pathPuppet;

        logFile.close();
        return 0;
    }
    pathPuppetUpdate[0] = '\0';

    std::wstring path = pathPuppet;
    path += L"Puppet.exe";

    HRESULT hr = S_OK;
    HRESULT comInit = E_FAIL;
    INetFwProfile* fwProfile = NULL;

    // Initialize COM.
    comInit = CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (comInit != RPC_E_CHANGED_MODE) {
        if (FAILED(comInit)) {
            logFile << "CoInitializeEx failed" << std::endl;
            std::cerr << "CoInitializeEx failed" << std::endl;

            delete[] pathPuppet;

            logFile.close();
            return 0;
        }
    }

    hr = WindowsFirewallInitialize(&fwProfile);
    if (FAILED(hr)) {
        if (SUCCEEDED(comInit))  CoUninitialize();

        delete[] pathPuppet;

        logFile.close();
        return 0;
    }

    hr = WindowsFirewallAddApp(fwProfile, path.c_str(), L"Puppet");
    if (FAILED(hr)) {
        logFile << "WindowsFirewallAddApp failed" << std::endl;
        std::cerr << "WindowsFirewallAddApp failed" << std::endl;

        if (fwProfile != NULL) fwProfile->Release();
        if (SUCCEEDED(comInit))  CoUninitialize();
        delete[] pathPuppet;

        logFile.close();
        return 0;
    }

    if (SUCCEEDED(comInit))  CoUninitialize();
    delete[] pathPuppet;

    return 1;
}

int main(int argc, char* argv[]) {  
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    LPWSTR ExecutablePath = new WCHAR[MAX_PATH];
    GetModuleFileName(NULL, ExecutablePath, 260);

    for (int i = (int)wcslen(ExecutablePath) - 1; i >= 0; i--) {
        if (ExecutablePath[i] == '\\') {
            ExecutablePath[i + 1] = '\0';
            break;
        }
    }
    if (!ExecutablePath) return 1;
    if (_wchdir(ExecutablePath) != 0) return 1;

    std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app };
    if (argc > 1 && strcmp(argv[1], "service") == 0) return InstallService();
    if (argc > 1 && strcmp(argv[1], "run") == 0) return RunNormal();

    SC_HANDLE scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (scmHandle == NULL) return 1;

    SERVICE_TABLE_ENTRY serviceTable[] = {
        { (LPWSTR)SERVICE_NAME, ServiceMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcher(serviceTable)) {
        CloseServiceHandle(scmHandle);
       
        logFile << "Could not start service error: " << GetLastError() << std::endl;
        std::cout << "Could not start service error: " << GetLastError() << std::endl;

        logFile << std::endl;
        logFile.close();

        return 1;
    }
    CloseServiceHandle(scmHandle);
    

    logFile << std::endl;
    logFile.close();
    return 0;
}




