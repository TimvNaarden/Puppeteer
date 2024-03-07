#pragma once

#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

std::string WriteJson(int input);
std::string WriteJson(float input);
std::string WriteJson(double input);
std::string WriteJson(short input);
std::string WriteJson(long input);
std::string WriteJson(unsigned int input);
std::string WriteJson(unsigned short input);
std::string WriteJson(unsigned long input);
std::string WriteJson(unsigned long long input);
std::string WriteJson(char input);
std::string WriteJson(char* input);
std::string WriteJson(const char* input);
std::string WriteJson(std::string input);
std::string WriteJson(bool input);

template <typename T> std::string WriteJson(std::vector<T> input) {
  std::string json = "[";
  for (int i = 0; i < input.size(); i++) {
    json += WriteJson(input[i]);
    if (i != input.size() - 1) {
      json += ",";
    }
  }
  json += "]";
  return json;
}

template <typename K, typename V>
std::string WriteJson(std::unordered_map<K, V> input) {
  std::string json = "{";

  char first = 0;

  for (auto &it : input) {
    json += first + WriteJson(it.first) + ":" + WriteJson(it.second);
    first = ',';
  }
  json += "}";
  return json;
}

template <typename K, typename V> std::string WriteJson(std::map<K, V> input) {
  std::string json = "{";

  int first = 1;
  for (auto &it : input) {
    if (first) {
      json += WriteJson(it.first) + ":" + WriteJson(it.second);
      first = 0;
    } else
      json += ',' + WriteJson(it.first) + ":" + WriteJson(it.second);
  }

  json += "}";
  return json;
}
