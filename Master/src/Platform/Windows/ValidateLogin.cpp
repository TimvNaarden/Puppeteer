#include <pch.h>
#include <Windows.h>

namespace Puppeteer {
    static bool authenticateUser(const std::string& username, const std::string& password, const std::string& domain = "") {
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
}