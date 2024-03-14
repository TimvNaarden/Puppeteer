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
            std::cerr << "Failed to fetch config file: " << std::endl;
            return;
        }
        curl_easy_cleanup(curl);
    }
    else {
        std::cerr << "Failed to initialize cURL." << std::endl;
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

static void RunOnStartup() {
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
}

int main(int argc, char* argv[]) {
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    //RunOnStartup();
    GetPuppetVersion();
    UpdatePuppet();
}