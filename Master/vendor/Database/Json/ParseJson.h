#pragma once
#include <any>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
#include <unordered_map>

template <typename T> struct is_vector : std::false_type {};
template <typename T, typename A> struct is_vector<std::vector<T, A>> : std::true_type {};

template <typename T> struct is_map : std::false_type {};
template <typename K, typename V, typename C, typename A> struct is_map<std::map<K, V, C, A>> : std::true_type {};

template <typename T> struct is_umap : std::false_type {};
template <typename K, typename V, typename C, typename A> struct is_umap<std::unordered_map<K, V, C, A>> : std::true_type {};

template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value && std::is_same_v<T, int>, int> ParseJson(std::string input) { return stoi(input); }
template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value && std::is_same_v<T, short>, short> ParseJson(std::string input) { return static_cast<short>(stoi(input)); }
template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value && std::is_same_v<T, unsigned int>, unsigned int> ParseJson(std::string input) { return static_cast<unsigned int>(stoul(input)); }
template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value && std::is_same_v<T, unsigned short>, unsigned short> ParseJson(std::string input) { return static_cast<unsigned short>(stoul(input)); }
template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value && std::is_same_v<T, bool>, bool> ParseJson(std::string input) { return input == "true"; }
template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value && std::is_same_v<T, double>, double> ParseJson(std::string input) { return stod(input); }
template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value && std::is_same_v<T, long double>, long double> ParseJson(std::string input) { return stold(input); }
template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value && std::is_same_v<T, float>, float> ParseJson(std::string input) { return stof(input); }
template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value && std::is_same_v<T, long>, long> ParseJson(std::string input) { return stol(input); }
template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value && std::is_same_v<T, unsigned long>, unsigned long> ParseJson(std::string input) { return stoul(input); }
template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value && std::is_same_v<T, long long>, long long> ParseJson(std::string input) { return stoll(input); }
template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value && std::is_same_v<T, unsigned long long>, unsigned long long> ParseJson(std::string input) { return stoull(input); }

template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value && std::is_same_v<T, std::string>, std::string> ParseJson(std::string input) {
  std::stringstream result;
  std::string sChars = "\"\\/bfnrt";
  std::string dsChars = "\"\\/\b\f\n\r\t";
  int escape = 0;
  for (int i = 1; i < input.length(); i++) {
    if (escape) {
      escape = false;
      if (input[i] == 'u') {
        result << static_cast<char>(stoi(input.substr(i + 1, 4), 0, 16));
        i += 4;
      } else if (sChars.find(sChars.find(input[i]) != std::string::npos)) {
        result << dsChars[sChars.find(input[i])];
      } else {
        result << input[i];
      }
    } else if (input[i] == '\"') {
      break;
    } else if (input[i] == '\\') {
      escape = 1;
    } else {
      result << input[i];
    }
  }
  return result.str();
}

template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value&& std::is_same_v<T, char*>, char*> ParseJson(std::string input) {
    return ParseJson<std::string>(input).data();
}

template <typename T> std::enable_if_t<!is_map<T>::value && !is_vector<T>::value&& std::is_same_v<T, const char*>, const char*> ParseJson(std::string input) {
    return ParseJson<std::string>(input).c_str();
}

template <typename T> std::enable_if_t<is_vector<T>::value, T> ParseJson(std::string input) {
  T result;
  int inString = 0;
  int start = 1;
  int left = 0, right = 0;
  for (int i = 1; i < input.length(); i++) {
    if (!inString) {
      if (input[i] == '{' || input[i] == '[')
        left++;
      if (input[i] == '}' || input[i] == ']')
        right++;
    }
    if (input[i] == '\"') {
        inString = !inString;
    }

    if (input[i] == '\\') {
      if (input[i + 1] == 'u') {
        i = i + 5;
        continue;
      }
      i++;
      continue;
    }

    if ((input[i] == ',' && !inString && left == right) || i == (input.length() -1)) {
      result.push_back(ParseJson<typename T::value_type>(input.substr(start, i - start)));
      start = i + 1;
    }
  }
  return result;
}
template <typename T> std::enable_if_t<is_map<T>::value || is_umap<T>::value, T> ParseJson(std::string input) {
  T result;
  int left = 0, right = 0;
  int inString = 0;
  int start = 1;
  T::key_type key{};
  for (int i = 1; i < input.length(); i++) {
    if (!inString) {
      if (input[i] == '{' || input[i] == '[')
        left++;
      if (input[i] == '}' || input[i] == ']')
        right++;
    }
    if (input[i] == '\"') {
      inString = !inString;
      continue;
    }

    if (input[i] == '\\') {
      if (input[i + 1] == 'u') {
        i = i + 5;
        continue;
      }
      i++;
      continue;
    }
    if (input[i] == ':' && !inString) {
      key = ParseJson<typename T::key_type>(input.substr(start, i - start));
      start = i + 1;
    } else if ((input[i] == ',' && !inString && left == right) || i == (input.length() - 1)) {
      result.emplace(key, ParseJson<typename T::mapped_type>(input.substr(start, i - start)));
      start = i + 1;
    }
  }
  return result;
}
