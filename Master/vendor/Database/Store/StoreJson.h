#pragma once
#include "../Json/ParseJson.h"
#include "../Json/WriteJson.h"
#include <fstream>
#include <string>
#include <vector>

#ifdef PLATFORM_LINUX
#include <sys/stat.h>
#endif // PLATFORM_LINUX

#ifdef PLATFORM_WINDOWS
#include <direct.h>
#endif // PLATFORM_WINDOWS

// @returns 0 if file exists
int CheckJsonTable(std::string table);

// @returns 0 if succesfull
int CreateJsonTable(std::string table);

// @returns 0 if succesfull
int InsertJsonTable(std::string table, std::string input);

// @returns 0 if succesfull
int RemoveJsonTable(std::string table, std::string input);

// @returns 0 if succesfull
int DeleteJsonTable(std::string table);

// @returns 0 if succesfull
int ReplaceJsonTable(std::string table, std::string condition, std::string input);

// @returns empty vector if error
template <typename T> std::vector<T> ExtractJsonTable(std::string table) {
  std::vector<T> result;
  if (CheckJsonTable(table))
    return result;

  std::string line;
  std::ifstream File("Data/Json/" + table);

  while (getline(File, line)) {
    result.push_back(ParseJson<T>(line));
  }
  return result;
}

// @returns NULL if error
template <typename T, class C> std::vector<T> ExtractJsonTable(std::string table, C pair) {
  std::vector<T> result;
  if (CheckJsonTable(table))
    return result;

  std::string filter = WriteJson(pair);

  std::string line;
  std::ifstream File("Data/Json/" + table);

  while (getline(File, line)) {
    if (line.find(filter))
      result.push_back(ParseJson<T>(line));
  }
  return result;
}
