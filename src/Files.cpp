// =============================================================================
// Created on Thu Feb 29 2024 21:40:03
// Author: Mukai (Tom Notch) Yu
// Email: myual@connect.ust.hk
// Affiliation: Hong Kong University of Science and Technology
//
// Copyright Ⓒ 2024 Mukai (Tom Notch) Yu
// =============================================================================

#include <boost/functional/hash.hpp>
#include "file_utils/Files.hpp"

using namespace std;

ostream& operator<<(ostream& os, const Key& key)
{
  visit([&os](auto&& arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (is_same_v<T, string>)
    {
      os << arg;
    }
    else if constexpr (is_same_v<T, int>)
    {
      os << arg;
    }
  },
        key);
  return os;
}

ostream& operator<<(ostream& os, const NodeType& nodeType)
{
  switch (nodeType)
  {
    case NodeType::leaf: {
      os << "leaf";
      break;
    }
    case NodeType::list: {
      os << "list";
      break;
    }
    case NodeType::map: {
      os << "map";
      break;
    }
  }
  return os;
}

bool AllKeywordsExist(const vector<string>& Keywords, const vector<string>& keys)
{
  for (const string& keyword : Keywords)
  {
    if (find(keys.begin(), keys.end(), keyword) == keys.end())
    {
      return false;
    }
  }
  return true;
}

string ExpandUser(const string& sProbePath)
{
  const char* home = getenv("HOME");
  CHECK(home != NULL) << "$HOME variable not set!";

  string sExpandPath;
  if (sProbePath[0] == '~')
  {
    sExpandPath = string(home) + sProbePath.substr(1, sProbePath.size() - 1);
  }
  else
  {
    sExpandPath = sProbePath;
  }
  return sExpandPath;
}

string ParsePath(const string& sProbePath, const string& sBasePath)
{
  CHECK(!sProbePath.empty()) << "Path " << sProbePath << " is empty!";

  filesystem::path probePath(sProbePath);

  filesystem::path expandPath(ExpandUser(sProbePath));                                     // expand ~ to usr home
  filesystem::path fullPath = filesystem::path(sBasePath) / probePath;                     // join base path and probe path
  filesystem::path projectBasePath = filesystem::path(S_PROJECT_BASE_FOLDER) / probePath;  // join project base folder and probe path
  if (expandPath.is_absolute() && filesystem::exists(expandPath))                          // expanded absolute path
  {
    return expandPath.string();
  }
  else if (filesystem::exists(fullPath))  // joined relative path
  {
    return fullPath.string();
  }
  else if (filesystem::exists(projectBasePath))  // joined project base path
  {
    return projectBasePath.string();
  }
  else
  {
    return "";
  }
}

File::File(const string& sFilePath)
{
  this->pFile = ReadFile(sFilePath);
}

File::File(const File& other)
{
  this->nodeType = other.nodeType;
  this->pFile = other.pFile;  // the elegancy of std::shared_ptr
}

File::~File()
{
  switch (this->nodeType)  // safely recursively explicitly call destructor for the tree structure
  {
    case NodeType::leaf: {
      break;
    }
    case NodeType::list: {
      for (std::shared_ptr<File>& node : *static_pointer_cast<List>(this->pFile))
      {
        node.reset();
      }
      break;
    }
    case NodeType::map: {
      for (auto& node : *static_pointer_cast<Map>(this->pFile))
      {
        static_pointer_cast<File>(node.second).reset();
      }
      break;
    }
  }
  this->pFile.reset();
}

const File& File::operator[](const Key& key) const
{
  switch (this->nodeType)
  {
    case NodeType::leaf: {
      CHECK(this->nodeType != NodeType::leaf) << "Index " << key << " invalid since the substructure is neither a list nor a map!";
      return *std::shared_ptr<File>(nullptr);  // for safety
      break;
    }
    case NodeType::list: {
      const int& iCurrentKey = std::get<int>(key);  // get int out of the variant Key
      const List& list = *static_pointer_cast<List>(this->pFile);
      CHECK(iCurrentKey >= 0 && iCurrentKey < list.size()) << "Index " << iCurrentKey << " out of range!";
      return *(list[iCurrentKey]);
      break;
    }
    case NodeType::map: {
      const string& sCurrentKey = std::get<string>(key);  // get string out of the variant Key
      Map& map = *static_pointer_cast<Map>(this->pFile);  // no const since: candidate function not viable: 'this' argument has type 'const Scheduler::Map' (aka 'const unordered_map<basic_string<char>, std::shared_ptr<Scheduler::File> >'), but method operator[] is not marked const
      CHECK(map.find(sCurrentKey) != map.end()) << "Key " << sCurrentKey << " does not exist!";
      return *(map[sCurrentKey]);
      break;
    }
    default: {
      CHECK(this->nodeType == NodeType::leaf || this->nodeType == NodeType::list || this->nodeType == NodeType::map) << "Unsupported file node type " << this->nodeType;
      return *std::shared_ptr<File>(nullptr);  // for safety
    }
  }
}

const std::shared_ptr<void> File::ReadFile(const string& sFilePath)
{
  string sParsedPath = ParsePath(sFilePath);
  CHECK(!sParsedPath.empty()) << "Path " << sFilePath << " does not exist!";
  filesystem::path parsedPath(sParsedPath);  // NOTE: this string can disappear due to scope

  filesystem::path fileExtension = parsedPath.extension();

  this->pFile.reset();  // safely delete the underlying structure if File::ReadFile is called second time

  if (fileExtension == ".yaml" || fileExtension == ".yml")
  {
    this->pFile = ReadYAML(sParsedPath);  // config file
  }
  else if (fileExtension == ".csv")
  {
    this->pFile = ReadCSV(sParsedPath);  // simply string now
  }
  else if (fileExtension == ".txt")
  {
    this->pFile = ReadTxt(sParsedPath);  // simply string now
  }
  else if (fileExtension == "")  // folder
  {
    this->nodeType = NodeType::leaf;
    this->pFile = std::make_shared<string>(sFilePath);
  }
  else  // unsupported file type, stored as string
  {
    LOG(WARNING) << "Path " << parsedPath.string() << "'s file type " << fileExtension << " is not supported";
    this->nodeType = NodeType::leaf;
    this->pFile = std::make_shared<string>(sFilePath);
  }

  return this->pFile;
}

const std::shared_ptr<void> File::ReadYAML(const string& sParsedPath)
{
  YAML::Node yamlNode = YAML::LoadFile(sParsedPath);
  return this->GetItem(yamlNode, filesystem::path(sParsedPath).parent_path().string());  // assign pointer to pFile for safety
}

const std::shared_ptr<void> File::ReadCSV(const string& sParsedPath)
{
  return std::make_shared<string>(sParsedPath);
}

struct pair_hash
{
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2>& p) const
  {
    std::size_t seed = 0;
    boost::hash_combine(seed, p.first);
    boost::hash_combine(seed, p.second);
    return seed;
  }
};

const std::shared_ptr<void> File::ReadTxt(const string& sParsedPath)
{
  return std::make_shared<string>(sParsedPath);
}

const std::shared_ptr<void> File::GetItem(const YAML::Node& node, const string& sYamlBasePath)
{
  switch (node.Type())
  {
    case YAML::NodeType::Undefined:  // should not happen
    {
      this->nodeType = NodeType::leaf;
      return std::shared_ptr<void>(nullptr);
      break;
    }
    case YAML::NodeType::Null: {
      this->nodeType = NodeType::leaf;
      return std::shared_ptr<void>(nullptr);
      break;
    }
    case YAML::NodeType::Scalar: {
      this->nodeType = NodeType::leaf;
      try  // try to convert to int first
      {
        return std::make_shared<int>(node.as<int>());
      }
      catch (const YAML::BadConversion& i)
      {
      }

      try  // try to convert to float
      {
        return std::make_shared<float>(node.as<float>());
      }
      catch (const YAML::BadConversion& f)
      {
      }

      try  // try to convert to bool
      {
        return std::make_shared<bool>(node.as<bool>());
      }
      catch (const YAML::BadConversion& b)
      {
      }

      try  // try to convert to string
      {
        string sParsedPath = ParsePath(node.as<string>(), sYamlBasePath);
        if (sParsedPath.empty())
        {
          return std::make_shared<string>(node.as<string>());
        }
        else
          return ReadFile(sParsedPath);  // a file to be read as new node
      }
      catch (const YAML::BadConversion& s)
      {
      }
      break;
    }
    case YAML::NodeType::Sequence:  // list
    {
      this->nodeType = NodeType::list;
      std::shared_ptr<List> pList(new List);
      for (const YAML::Node& subnode : node)
      {
        std::shared_ptr<File> pSubFile(new File);  // every sub-item will be a new File
        pSubFile->pFile = pSubFile->GetItem(subnode, sYamlBasePath);
        pList->push_back(pSubFile);
      }
      return pList;
      break;
    }
    case YAML::NodeType::Map:  // map
    {
      this->nodeType = NodeType::map;
      std::shared_ptr<Map> pMap(new Map);
      for (const auto& pair : node)
      {
        std::shared_ptr<File> pSubFile(new File);  // every sub-item will be a new File
        pSubFile->pFile = pSubFile->GetItem(pair.second, sYamlBasePath);
        (*pMap)[pair.first.as<string>()] = pSubFile;
      }
      return pMap;
      break;
    }
  }
  // should not execute to here
  CHECK(false) << "Unsupported file node type " << node.Type();
  return std::shared_ptr<void>(nullptr);
}

string Indent(const int& iIndent)
{
  string sIndent = "";
  for (int i = 0; i < iIndent; ++i)
  {
    sIndent += "\t";
  }
  return sIndent;
}

const string File::Print(const int& iIndent) const
{
  switch (this->nodeType)
  {
    case NodeType::leaf: {
      return "leaf\n";
      break;
    }
    case NodeType::list: {
      const List& list = *static_pointer_cast<List>(this->pFile);
      string sList = "\n";
      for (int index = 0; const auto& node : list)
      {
        sList += Indent(iIndent) + to_string(index++) + ":\t" + node->Print(iIndent + 1);
      }
      return sList;
      break;
    }
    case NodeType::map: {
      const Map& map = *static_pointer_cast<Map>(this->pFile);
      string sMap = "\n";
      for (const auto& pair : map)
      {
        sMap += Indent(iIndent) + pair.first + ":\t" + pair.second->Print(iIndent + 1);
      }
      return sMap;
      break;
    }
    default: {
      CHECK(this->nodeType == NodeType::leaf || this->nodeType == NodeType::list || this->nodeType == NodeType::map) << "Unsupported file node type " << this->nodeType;
      return "";
    }
  }
}

ostream& operator<<(ostream& os, const File& file)
{
  return os << file.Print();
}
