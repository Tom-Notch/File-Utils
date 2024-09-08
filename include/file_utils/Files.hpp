// =============================================================================
// Created on Thu Feb 29 2024 21:40:16
// Author: Mukai (Tom Notch) Yu
// Email: myual@connect.ust.hk
// Affiliation: Hong Kong University of Science and Technology
//
// Copyright Ⓒ 2024 Mukai (Tom Notch) Yu
// =============================================================================

#pragma once

#include <file_utils/visibility_control.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// google log
#include <glog/logging.h>

// yaml-cpp
#include <yaml-cpp/yaml.h>

// csv-parser
#include "csv.hpp"

using namespace std;

// inline is recommended for C++17 or after since it won't be multiply defined
inline const string& S_PROJECT_BASE_FOLDER = filesystem::path(__FILE__).parent_path().parent_path().string();

class File;  // pre-define for type definition

typedef variant<string, int> Key;
ostream& operator<<(ostream& os, const Key& key);
typedef std::unordered_map<Key, std::shared_ptr<File>> Dict;    // each item is a File
typedef vector<std::shared_ptr<File>> List;                     // each item is a File
typedef std::unordered_map<string, std::shared_ptr<File>> Map;  // each item is a File

enum class NodeType
{
  leaf,
  list,
  map
};

/**
 * @brief cout << NodeType;
 *
 * @param os
 * @param nodeType
 * @return ostream&
 */
ostream& operator<<(ostream& os, const NodeType& nodeType);

template <typename Key, typename Value>
ostream& operator<<(ostream& os, const std::unordered_map<Key, Value>& map)
{
  os << endl
     << "{ " << endl;
  for (const auto& pMap : map)
  {
    os << "\t" << pMap.first << ": " << pMap.second << endl;
  }
  os << "}" << endl;
  return os;
}

template <typename Key, typename Value>
ostream& operator<<(ostream& os, const map<Key, Value>& map)
{
  os << endl
     << "{ " << endl;
  for (const auto& pMap : map)
  {
    os << "\t" << pMap.first << ": " << pMap.second << endl;
  }
  os << "}" << endl;
  return os;
}

/**
 * @brief Check if all keywords exist in the keys of a map
 *
 * @param Keywords vector of string
 * @param keys vector of string
 * @return true if all presents
 * @return false if any one missing
 */
bool AllKeywordsExist(const vector<string>& Keywords, const vector<string>& keys);

/**
 * @brief Expands the path with home variable "~" to full absolute path
 *
 * @param sProbePath Input path potentially with "~"
 * @return string Expanded full absolute path without "~"
 */
string ExpandUser(const string& sProbePath);

/**
 * @brief Parse the sProbePath, check whether it's a full absolute path, relative path to the given sBasePath, or relative path to project base folder
 *
 * @param sProbePath string of probe path
 * @param sBasePath base folder to check the relative path
 * @return string full absolute path if any of the above resolves to a valid path, otherwise empty
 */
string ParsePath(const string& sProbePath, const string& sBasePath = "");

class File
{
public:
  File(const string& sFilePath);

  /**
   * @brief Construct a new File object
   *        Deprecated, should use the previous constructor, this is only to enable make_shared<File>()
   *
   */
  File() = default;

  /**
   * @brief Copy constructor to allow File a = b or File a(b);
   *
   * @param other
   */
  File(const File& other);

  ~File();

  template <typename T>
  std::shared_ptr<T> get(const Key& currentKey) const
  {
    CHECK(this->nodeType != NodeType::leaf) << "Index " << currentKey << " invalid since the substructure is neither a leaf nor a map!";

    switch (this->nodeType)
    {
      case NodeType::leaf: {
        CHECK(false) << "Index " << currentKey << " invalid since the substructure is neither a leaf nor a map!";
        break;
      }
      case NodeType::list: {
        const int& iCurrentKey = std::get<int>(currentKey);  // get int out of the variant Key
        const List& list = *static_pointer_cast<List>(this->pFile);
        CHECK(iCurrentKey >= 0 && iCurrentKey < list.size()) << "Index " << iCurrentKey << " out of range!";
        return std::shared_ptr<T>(new T(*reinterpret_cast<T*>(list[iCurrentKey]->pFile.get())));
        break;
      }
      case NodeType::map: {
        const string& sCurrentKey = std::get<string>(currentKey);  // get string out of the variant Key
        Map& map = *static_pointer_cast<Map>(this->pFile);         // no const since: candidate function not viable: 'this' argument has type 'const Scheduler::Map' (aka 'const unordered_map<basic_string<char>, std::shared_ptr<Scheduler::File> >'), but method operator[] is not marked const
        CHECK(map.find(sCurrentKey) != map.end()) << "Key " << sCurrentKey << " does not exist!";
        return std::shared_ptr<T>(new T(*reinterpret_cast<T*>((map[sCurrentKey])->pFile.get())));
        break;
      }
    }
  }

  /**
   * @brief Variadic template version of operator[], example usage: file.get<string>("a", 0, "b", 1)
   *
   * @param T
   * @param Keys
   * @param currentKey
   * @param otherKeys
   * @return std::shared_ptr<T>
   */
  template <typename T, typename... Keys>
  std::shared_ptr<T> get(const Key& currentKey, const Keys&... otherKeys) const
  {
    CHECK(this->nodeType != NodeType::leaf) << "Index " << currentKey << " invalid since the substructure is neither a leaf nor a map!";

    switch (this->nodeType)
    {
      case NodeType::leaf: {
        CHECK(false) << "Index " << currentKey << " invalid since the substructure is neither a leaf nor a map!";
        break;
      }
      case NodeType::list: {
        const int& iCurrentKey = std::get<int>(currentKey);  // get int out of the variant Key
        const List& list = *static_pointer_cast<List>(this->pFile);
        CHECK(iCurrentKey >= 0 && iCurrentKey < list.size()) << "Index " << iCurrentKey << " out of range!";
        return list[iCurrentKey]->get<T>(otherKeys...);
        break;
      }
      case NodeType::map: {
        const string& sCurrentKey = std::get<string>(currentKey);  // get string out of the variant Key
        Map& map = *static_pointer_cast<Map>(this->pFile);         // no const since: candidate function not viable: 'this' argument has type 'const Scheduler::Map' (aka 'const unordered_map<basic_string<char>, std::shared_ptr<Scheduler::File> >'), but method operator[] is not marked const
        CHECK(map.find(sCurrentKey) != map.end()) << "Key " << sCurrentKey << " does not exist!";
        return (map[sCurrentKey])->get<T>(otherKeys...);
        break;
      }
    }
  }

  /**
   * @brief Access a SubFile, can use with file["a"][0]["b"][1], generally needs to call as<T>() for reference or get<T>() for std::shared_ptr in the end to get the underlying object
   *
   * @param key can be int or string
   * @return const File&
   */
  const File& operator[](const Key& key) const;

  /**
   * @brief Cast the pFile to T type and return a reference to it, not using const T& since might not be feasible for pytorch module
   *
   * @param T
   * @return T& reference to underlying data
   */
  template <typename T>
  T& as() const  // the "so-called" const
  {
    return *static_pointer_cast<T>(this->pFile);
  }

  /**
   * @brief safe backdoor for std::shared_ptr
   *
   * @param T
   * @return std::shared_ptr<T>
   */
  template <typename T>
  std::shared_ptr<T> get() const
  {
    return static_pointer_cast<T>(this->pFile);
  }

  /**
   * @brief Actual implementation of operator<<
   *
   * @param iIndent # indentation
   * @return const string
   */
  const string Print(const int& iIndent = 0) const;

  /**
   * @brief file logging utility, example usage: cout << file << ... ;
   *
   * @param os
   * @param file
   * @return ostream&
   */
  friend ostream& operator<<(ostream& os, const File& file);

private:
  NodeType nodeType = NodeType::leaf;
  std::shared_ptr<void> pFile = std::shared_ptr<void>(nullptr);

  /**
   * @brief: Base reading function that dispatches reading job to specific functions based on file extension
   *         Current support format:
   *         1. Configuration file: .yaml, .yml
   *         2. task graph file: .csv
   * @return:
   *         A std::shared_ptr pointing to any possible file node, can be
   *         1. std::shared_ptr<void> for nested structure, either pointing to a List or a Map
   *         2. std::shared_ptr<void>(nullptr) if format doesn't support
   */
  const std::shared_ptr<void> ReadFile(const string& sFilePath);

  /**
   * @brief Recursively read YAML config
   *
   * @param sParsedPath parsed path from ReadFile
   * @return const std::shared_ptr<void>
   */
  const std::shared_ptr<void> ReadYAML(const string& sParsedPath);

  /**
   * @brief Read CSV file, constructs task graph
   *
   * @param sParsedPath parsed path from ReadFile
   * @return const std::shared_ptr<void>
   */
  const std::shared_ptr<void> ReadCSV(const string& sParsedPath);

  /**
   * @brief Read txt file, constructs resource graph
   *
   * @param sParsedPath parsed path from ReadFile
   * @return const std::shared_ptr<void>
   */
  const std::shared_ptr<void> ReadTxt(const string& sParsedPath);

  /**
   * @brief Recursively get the nodes of a yaml, called by ReadYAML
   *
   * @param node root node of the yaml file
   * @param sYamlBasePath base folder of a customized path, can be project-based
   * @return const std::shared_ptr<void>
   */
  const std::shared_ptr<void> GetItem(const YAML::Node& node, const string& sYamlBasePath);
};  // class File
