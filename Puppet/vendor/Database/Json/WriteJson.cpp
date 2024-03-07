#include "WriteJson.h"

std::string WriteJson(int input) { return std::to_string(input); }

std::string WriteJson(float input) { return std::to_string(input); }

std::string WriteJson(double input) { return std::to_string(input); }

std::string WriteJson(short input) { return std::to_string(input); }

std::string WriteJson(long input) { return std::to_string(input); }

std::string WriteJson(unsigned int input) { return std::to_string(input); }

std::string WriteJson(unsigned short input) { return std::to_string(input); }

std::string WriteJson(unsigned long input) { return std::to_string(input); }

std::string WriteJson(unsigned long long input) { return std::to_string(input); }

#include <string>

std::string WriteJson(char input) {
    std::string result = "";
    std::string sChars = "\"\\/\b\f\n\r\t";
    std::string dsChars = "\"\\\\/bfnrt";

    size_t found = sChars.find(input);
    if (found != std::string::npos) {
        result += '\\';
        result += dsChars[found];
    }
    else {
        result += input;
    }

    return "\"" + result + "\"";
}

std::string WriteJson(std::string input) {
    std::string result = "";
    std::string sChars = "\"\\/\b\f\n\r\t";
    std::string dsChars = "\"\\\\/bfnrt";

    for (char c : input) {
        size_t found = sChars.find(c);
        if (found != std::string::npos) {
            result += '\\';
            result += dsChars[found];
        }
        else {
            result += c;
        }
    }

    return "\"" + result + "\"";
}

std::string WriteJson(const char* input) {
    std::string result = "";
    std::string sChars = "\"\\/\b\f\n\r\t";
    std::string dsChars = "\"\\\\/bfnrt";

    for (const char* ptr = input; *ptr != '\0'; ptr++) {
        size_t found = sChars.find(*ptr);
        if (found != std::string::npos) {
            result += '\\';
            result += dsChars[found];
        }
        else {
            result += *ptr;
        }
    }

    return "\"" + result + "\"";
}

std::string WriteJson(char* input) {
    std::string result = "";
    std::string sChars = "\"\\/\b\f\n\r\t";
    std::string dsChars = "\"\\\\/bfnrt";

    for (char* ptr = input; *ptr != '\0'; ptr++) {
        size_t found = sChars.find(*ptr);
        if (found != std::string::npos) {
            result += '\\';
            result += dsChars[found];
        }
        else {
            result += *ptr;
        }
    }

    return "\"" + result + "\"";
}

std::string WriteJson(bool input) { return input ? "true" : "false"; }
