#include <curl/curl.h>
#include <iostream>
#include <Json/ParseJson.h>
#include <map>
#include <string>

#include <fstream>
#include <strsafe.h>
#include <urlmon.h>
#include <Windows.h>
#pragma comment(lib, "urlmon.lib")

#include <WtsApi32.h>
#pragma comment(lib, "Wtsapi32.lib")
#define SERVICE_NAME  L"Puppet Service"

struct Version {
    int Major;
    int Minor;
    int Revision;
};

static std::string version = "V0.0.0";
std::fstream logFile{ "PuppetUpdateLog.txt", std::ios::app };

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
DWORD GetWinlogonPIDInSession(DWORD dwSessionId);
BOOL ServiceExists(const TCHAR* serviceName);

void UpdatePuppet();
void GetPuppetVersion();
static Version ParseVersion(std::string version);
static bool CompareVersions(std::string version1, std::string version2);

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    // Go into the directory of the executable so that log file is easy to find
    LPWSTR ExecutablePath = new WCHAR[MAX_PATH];
    GetModuleFileName(NULL, ExecutablePath, 260);

    for (int i = wcslen(ExecutablePath) - 1; i >= 0; i--) {
        if (ExecutablePath[i] == '\\') {
            ExecutablePath[i + 1] = '\0';
            break;
        }
    }
    if (!ExecutablePath) return;
    if (_wchdir(ExecutablePath) != 0) return;


    DWORD Status = E_FAIL;

    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

    if (g_StatusHandle == NULL) return;

    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) OutputDebugString(L"Puppet Service: ServiceMain: SetServiceStatus returned error");
    
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

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) OutputDebugString(L"Puppet Service: ServiceMain: SetServiceStatus returned error");

        return;
    }

    // Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)  OutputDebugString(L"Puppet Service: ServiceMain: SetServiceStatus returned error");

    GetPuppetVersion();
    UpdatePuppet();
    while (WTSGetActiveConsoleSessionId() == 0 || WTSGetActiveConsoleSessionId() == 0xFFFFFFFF) continue;
    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    if (hThread == NULL) {
		// Error creating worker thread
		// Tell service controller we are stopped and exit
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) OutputDebugString(L"Puppet Service: ServiceMain: SetServiceStatus returned error");

		return;
	}
    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(g_ServiceStopEvent);

    // Tell the service controller we are stopped
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) OutputDebugString(L"Puppet Service: ServiceMain: SetServiceStatus returned error");
  
    return;
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode) {
    switch (CtrlCode) {
    case SERVICE_CONTROL_STOP:

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING) break;

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(L"Puppet Service: ServiceCtrlHandler: SetServiceStatus returned error");
        }
        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);
        break;
    default:
        break;
    }
}

DWORD GetWinlogonPIDInSession(DWORD dwSessionId) {
    DWORD dwWinlogonPID = 0;
    PWTS_PROCESS_INFO pProcessInfo = NULL;
    DWORD dwProcessCount = 0;

    if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &dwProcessCount)) {
        for (DWORD i = 0; i < dwProcessCount; ++i) {
            if (_wcsicmp(pProcessInfo[i].pProcessName, L"winlogon.exe") == 0 && pProcessInfo[i].SessionId == dwSessionId) {
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

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam) {

    // Get the current session ID
    DWORD currentSessionId = WTSGetActiveConsoleSessionId();
    logFile << "Session ID " << currentSessionId << std::endl;
    int first = 1;

    while (true) {
        // Wait for a session change notification
        DWORD newSessionId = WTSGetActiveConsoleSessionId();

        if ((newSessionId != currentSessionId && newSessionId != 0xFFFFFFFF) || first) {
            first = 0;
            logFile << "Session ID " << newSessionId << " changed." << std::endl;
            currentSessionId = newSessionId;
            // Start an exe in the new session
            const wchar_t* exePath = L"Puppet.exe";
            DWORD winlogonPID = GetWinlogonPIDInSession(newSessionId);
            if (winlogonPID != 0) {
               logFile << "Winlogon.exe PID in session " << newSessionId << ": " << winlogonPID << std::endl;
            }
            else {
                logFile << "Failed to get winlogon.exe PID in session " << newSessionId << std::endl;
            }

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, winlogonPID);
            if (hProcess == NULL) {
                logFile << "Failed to open winlogon.exe process. Error: " << GetLastError() << std::endl;
                logFile.close();
                return 1;
            }

            // Get access token of winlogon.exe
            HANDLE hToken = NULL;
            if (!OpenProcessToken(hProcess, TOKEN_DUPLICATE, &hToken)) {
                logFile << "Failed to open token of winlogon.exe. Error: " << GetLastError() << std::endl;
                CloseHandle(hProcess);
                logFile.close();
                return 1;
            }

            // Duplicate the token to get a primary token
            HANDLE hPrimaryToken = NULL;
            if (!DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hPrimaryToken)) {
                logFile << "Failed to duplicate token. Error: " << GetLastError() << std::endl;
                CloseHandle(hToken);
                CloseHandle(hProcess);
                logFile.close();
                return 1;
            }

            // Close handles
            CloseHandle(hToken);
            CloseHandle(hProcess);

            // Start the process in the user's session with elevated privileges
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
                CloseHandle(hPrimaryToken);
            }
            else {
                logFile << "Process created." << std::endl;
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                CloseHandle(hPrimaryToken);
            }
        }
    }

    logFile.close();
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)  {
        ShowWindow(GetConsoleWindow(), SW_HIDE);
        GetPuppetVersion();
        UpdatePuppet();
    }

    return ERROR_SUCCESS;
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
    FILE* file;
    file = _popen("Puppet.exe -v", "r");
    if (file == NULL) {
		std::cerr << "Failed to open Puppet.exe"<< std::endl;
        return;
	}
    char buffer[128];
	std::string result = "";
    while (fgets(buffer, 128, file) != NULL) {
		result += buffer;
	}
	_pclose(file);
    if (!result._Starts_with("V")) return;
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
            return;
        }
        curl_easy_cleanup(curl);
    }
    else {
        logFile << "Failed to initialize cURL." << std::endl;
        return;
    }
    LPWSTR ExecutablePath = new WCHAR[MAX_PATH];
    GetModuleFileName(NULL, ExecutablePath, 260);

    for (int i = wcslen(ExecutablePath) - 1; i >= 0; i--) {
        if (ExecutablePath[i] == '\\') {
            ExecutablePath[i + 1] = '\0';
            break;
        }
    }
    if(!ExecutablePath) return;
    if(_wchdir(ExecutablePath) != 0) return;
    char* workdir = new char[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, ExecutablePath, -1, workdir, 256, NULL, NULL);
    logFile << "Working directory: " << workdir << std::endl;

    std::map<std::string, std::string> config = ParseJson<std::map<std::string, std::string>>(config_data);
    logFile << "Current version: " << version << std::endl;
    logFile << "New version: " << config["version"] << std::endl;

    if (CompareVersions(config["version"], version.substr(1, version.size() -1))) {
        logFile << "New version Found" << std::endl;
        std::cout << "New version Found" << std::endl;
        StringCchCatW(ExecutablePath, MAX_PATH, L"temp.zip");
        LPWSTR URL = new WCHAR[strlen(config["url"].c_str()) + 1];
        MultiByteToWideChar(CP_ACP, 0, config["url"].c_str(), -1, URL, strlen(config["url"].c_str()) + 1);
        HRESULT result = URLDownloadToFileW(NULL, URL, ExecutablePath, 0, NULL);
        system("tar -xf temp.zip");
        system("del temp.zip");
        logFile << "Puppet updated" << std::endl;
    }
}

int main(int argc, char* argv[]) {  
    if (argc > 1 && strcmp(argv[1], "service") == 0) {
        logFile << "Creating service" << std::endl;
        LPWSTR ExecutablePath = new WCHAR[MAX_PATH];
        GetModuleFileName(NULL, ExecutablePath, 260);
        if (!ServiceExists(SERVICE_NAME)) {
            SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
            if (!scm) {
                std::cerr << "Failed to open Service Control Manager" << std::endl;
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
                std::cerr << "Failed to create service: " << GetLastError() << std::endl;
                CloseServiceHandle(scm);
                return 1;
            }
            SERVICE_DESCRIPTION serviceDesc;
            serviceDesc.lpDescription = (LPWSTR)L"Client Side Service For Puppeteer, https://github.com/timvnaarden/puppeteer";
            ChangeServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, &serviceDesc);

            std::cout << "Service installed successfully!" << std::endl;

            CloseServiceHandle(service);
            CloseServiceHandle(scm);
            logFile << "Service created" << std::endl;
        }
        else {
            std::cout << "Service already exists!" << std::endl;
            logFile << "Service already exists" << std::endl;
        }

        return 0;
    }

    LPWSTR ExecutablePath = new WCHAR[MAX_PATH];
    GetModuleFileName(NULL, ExecutablePath, 260);

    for (int i = wcslen(ExecutablePath) - 1; i >= 0; i--) {
        if (ExecutablePath[i] == '\\') {
            ExecutablePath[i + 1] = '\0';
            break;
        }
    }
    if (!ExecutablePath) return 1;
    if (_wchdir(ExecutablePath) != 0) return 1;

    SC_HANDLE scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (scmHandle == NULL) return 1;

    SERVICE_TABLE_ENTRY serviceTable[] = {
        { SERVICE_NAME, ServiceMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcher(serviceTable))  logFile << "Could not start service error: " << GetLastError() << std::endl;;
    CloseServiceHandle(scmHandle);
    
    logFile << "Starting PuppetUpdate - Manual Exe" << std::endl;
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    GetPuppetVersion();
    UpdatePuppet();   
    logFile << "Starting Puppet" << std::endl;
    system("Puppet.exe");
    logFile << std::endl;
    logFile.close();
    return 0;
}
 

//sc create PuppetService binPath= "D:\OneDrive\Coding\C++\Puppeteer\PuppetUpdate\bin\Debug-windows-x86_64\PuppetUpdate\PuppetUpdate.exe" 

