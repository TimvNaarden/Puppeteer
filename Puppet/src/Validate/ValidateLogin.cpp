#include "ValidateLogin.h"

namespace Puppeteer {
    bool authenticateUser(std::string username, std::string password, std::string domain) {
        HANDLE token;

        // Use the LogonUser function to attempt to log in the user
        if (LogonUserA(
            username.c_str(),
            domain.c_str(),  // Use the local machine
            password.c_str(),
            LOGON32_LOGON_NETWORK,
            LOGON32_PROVIDER_DEFAULT,
            &token
        ) != 0) {
            // Authentication successful
            CloseHandle(token);
            // Delete the password
            SecureZeroMemory((void*)password.c_str(), password.length());
            return true;
        }
        else {
            // Authentication failed
            return false;
        }
    }

    bool userIsAdmin(std::string username, std::string domain)
    {
        std::wstring temp = std::wstring(username.begin(), username.end());
        LPCWSTR wszUserName = temp.c_str();

        std::wstring temp2 = std::wstring(domain.begin(), domain.end());
        LPCWSTR wszDomainName = temp2.c_str();

        LPLOCALGROUP_USERS_INFO_0 pGroupsInfo = NULL;
        DWORD dwEntriesRead = 0;
        DWORD dwTotalEntries = 0;
        DWORD dwError = 0;


        // Retrieve group information for the user
        dwError = NetUserGetGroups(wszDomainName, wszUserName, 0, (LPBYTE*)&pGroupsInfo, MAX_PREFERRED_LENGTH, &dwEntriesRead, &dwTotalEntries);

        // Check for errors
        if (dwError != NERR_Success) {
            if (pGroupsInfo != NULL) {
                NetApiBufferFree(pGroupsInfo);
            }
            return false;
        }

        for (DWORD i = 0; i < dwEntriesRead; ++i) {
            if (std::wcscmp(pGroupsInfo[i].lgrui0_name, L"Administrators") == 0) {
                return true;
            }
            if (std::wcscmp(pGroupsInfo[i].lgrui0_name, L"Domain Admin") == 0) {
                return true;
            }
        }

        // Free the buffer
        if (pGroupsInfo != NULL) {
            NetApiBufferFree(pGroupsInfo);
        }

        return false;
    }
}
