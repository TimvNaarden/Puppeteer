#include "StoreJson.h"

struct stat buffer;

int CheckJsonTable(std::string table) { return !(stat(("Data/Json/" + table).c_str(), &buffer) == 0); }

int CreateJsonTable(std::string table) {
  if (!CheckJsonTable(table)) {
    return 1;
  }
  std::ofstream MyFile("Data/Json/" + table);
  return CheckJsonTable(table);
}

int InsertJsonTable(std::string table, std::string input) {
  if (CheckJsonTable(table))
    return 1;
  std::fstream MyFile{"Data/Json/" + table, std::ios::app};
  MyFile << input << std::endl;
  MyFile.close();
  return !MyFile.good();
}

int RemoveJsonTable(std::string table, std::string input) {
  if (CheckJsonTable(table))
    return 1;

  std::fstream MyFile{"Data/Json/" + table, std::ios::in};
  std::stringstream Result;
  std::string line;

  while (getline(MyFile, line)) {
    if (line != input) {
      Result << line << std::endl;
    }
  }

  MyFile.close();
  std::ofstream ResultFile{"Data/Json/" + table};
  ResultFile << Result.str();
  return 0;
}

int DeleteJsonTable(std::string table) { return std::remove(("Data/Json/" + table).c_str()); }

int ReplaceJsonTable(std::string table, std::string condition, std::string input) {
  if (CheckJsonTable(table))
    return 1;

  std::fstream MyFile{"Data/Json/" + table, std::ios::in};
  std::stringstream Result;
  std::string line;

  while (getline(MyFile, line)) {
    if (line.find(condition) != std::string::npos)
      Result << input << std::endl;
    else
      Result << line << std::endl;
  }

  MyFile.close();
  std::ofstream ResultFile{"Data/Json/" + table};
  ResultFile << Result.str();
  return 0;
}
