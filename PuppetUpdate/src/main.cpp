#include <iostream>
#include <string>
#include <map>
#include <curl/curl.h>
#include <Json/ParseJson.h>

#include <Windows.h>
#include <urlmon.h>
#include <strsafe.h>
#pragma comment(lib, "urlmon.lib")

struct Version {
    int Major;
    int Minor;
    int Revision;
};

static std::string version = "V0.0.0";

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

#define SERVICE_NAME  L"Puppet Service"


void UpdatePuppet();
void GetPuppetVersion();
static Version ParseVersion(std::string version);

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
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

// ServiceControlHandler function
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

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam) {
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)  {
        ShowWindow(GetConsoleWindow(), SW_HIDE);
        GetPuppetVersion();
        UpdatePuppet();
    }

    return ERROR_SUCCESS;
}
static void GetPuppetVersion() {
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
            std::cerr << "Failed to fetch config file: " << std::endl;
            return;
        }
        curl_easy_cleanup(curl);
    }
    else {
        std::cerr << "Failed to initialize cURL." << std::endl;
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
    std::map<std::string, std::string> config = ParseJson<std::map<std::string, std::string>>(config_data);
    if (CompareVersions(config["version"], version.substr(1, version.size() -1))) {
        std::cout << "New version Found" << std::endl;
        StringCchCatW(ExecutablePath, MAX_PATH, L"temp.zip");
        LPWSTR URL = new WCHAR[strlen(config["url"].c_str()) + 1];
        MultiByteToWideChar(CP_ACP, 0, config["url"].c_str(), -1, URL, strlen(config["url"].c_str()) + 1);
        HRESULT result = URLDownloadToFileW(NULL, URL, ExecutablePath, 0, NULL);
        system("tar -xf temp.zip && del temp.zip");
    }
    system("Puppet.exe");
}

/* static void RunOnStartup() {
    HKEY hKey;
	LONG lResult;
	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
    if (lResult != ERROR_SUCCESS) {
		std::cerr << "Failed to open registry key" << std::endl;
		return;
	}
	LPWSTR ExecutablePath = new WCHAR[MAX_PATH];
	GetModuleFileName(NULL, ExecutablePath, 260);
	lResult = RegSetValueExW(hKey, L"Puppeteer", 0, REG_SZ, (LPBYTE)ExecutablePath, wcslen(ExecutablePath) * 2);
    if (lResult != ERROR_SUCCESS) {
		std::cerr << "Failed to set registry key" << std::endl;
		return;
	}
	RegCloseKey(hKey);
} */
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

int main(int argc, char* argv[]) {

    if (argc > 1 && strcmp(argv[1], "service") == 0) {
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
                SERVICE_WIN32_OWN_PROCESS, // Service type
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

            std::cout << "Service installed successfully!" << std::endl;

            CloseServiceHandle(service);
            CloseServiceHandle(scm);
        }
        else std::cout << "Service already exists!" << std::endl;
        return 0;
    }

    SC_HANDLE scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (scmHandle == NULL) return 1;

    SERVICE_TABLE_ENTRY serviceTable[] = {
        { SERVICE_NAME, ServiceMain },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcher(serviceTable))  std::cout << GetLastError() << std::endl;;
    CloseServiceHandle(scmHandle);
    
    
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    GetPuppetVersion();
    UpdatePuppet();   
    return 0;
}
 //sc create PuppetService binPath= "D:\OneDrive\Coding\C++\Puppeteer\PuppetUpdate\bin\Debug-windows-x86_64\PuppetUpdate\PuppetUpdate.exe" 

