#include <sstream>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "../include/indexer.h"
#include "../../../include/common/tokenizer.hpp"
#include "../../../include/common/normalizer.hpp"
#include "../../../include/common/porter2_stemmer.h"
#include "../../../include/common/loc.hpp"


Zone::Zone(const tinyxml2::XMLElement& tiElem) {
  name = getNecessaryTag(tiElem, "name")->GetText();
  //
  auto tag = tiElem.FirstChildElement("path");
  if (tag) {
    std::istringstream iss(tag->GetText());
    std::string pathTag;
    while(std::getline(iss, pathTag, ',')) {
      path.push_back(pathTag);
    }
  }
  else {
    path.push_back(name);
  }
  //
  tag = tiElem.FirstChildElement("optional");
  if (tag) {
    std::string text = tag->GetText();
    if (text == "true" || text == "True" || text == "1") {
      isOptional = true;
    }
    else if (text == "false" || text == "False" || text == "0") {
      isOptional = false;
    }
    else {
      throw Indexer::Exception("Cannot load config. Zone \"optional\" must be \"[Tt]rue|[Ff]alse|[01]\"");
    }
  }
  else {
    isOptional = false;
  }
}

const tinyxml2::XMLElement* getNecessaryTag(const tinyxml2::XMLElement& tiElem, const std::string& tag) {
  auto result = tiElem.FirstChildElement(tag.c_str());
  if (!result) {
    throw Indexer::Exception("There is no tag '" + tag + "'");
  }
  return result;
}


std::ostream& operator<<(std::ostream& os, const Zone& zone) {
  os << "{name: " << zone.name << ", path: [";
  int count = zone.path.size();
  for (const auto& i : zone.path) {
    os << i;
    if (--count != 0) {
      os << ", ";
    }
  }
  os << "]}";
  return os;
}




Indexer::Indexer(const std::string& configPath) {
  tinyxml2::XMLDocument doc;
  if (doc.LoadFile(configPath.c_str())) {
    throw Exception("Cannot load xml file '" + configPath + "'");
  }
  for (auto tag = doc.FirstChildElement(); tag != 0 ; tag = tag->NextSiblingElement()) {
    std::string tagName = tag->Name();
    if (tagName == "doc_id") {
      _docId = tag->GetText();
    }
    else if (tagName == "raw_data") {
      _rawDataPath = getNecessaryTag(*tag, "path")->GetText();
    }
    else if (tagName == "index_data") {
      _indexDataPath = getNecessaryTag(*tag, "path")->GetText();
      _plainIndexFile = getNecessaryTag(*tag, "plain_index")->GetText();
      _invertIndexFile = getNecessaryTag(*tag, "invert_index")->GetText();
    }
    else {
      std::vector<Zone*>* pZones;
      if (tagName == "text_zones") {
        pZones = &_textZones;
      }
      else if (tagName == "num_zones") {
        pZones = &_numZones;
      }
      else if (tagName == "trash_zones") {
        pZones = &_trashZones;
      }
      else {
        throw Exception("Failed to load config. Unsupported tag '" + tagName + "'");
      }
      for (auto zoneTag = tag->FirstChildElement(); zoneTag != 0 ; zoneTag = zoneTag->NextSiblingElement()) {
        pZones->push_back(new Zone(*zoneTag));
      }
    }
  }
}

Indexer::~Indexer() {
  for (const auto& it : _textZones) {
    delete it;
  }
  for (const auto& it : _numZones) {
    delete it;
  }
  for (const auto& it : _trashZones) {
    delete it;
  }
}


void Indexer::tmpPrint() {
  std::cout << "doc_id is '" << _docId << "'" << std::endl;
  std::cout << "text zones are:" << std::endl;
  for (const auto& zoneIt : _textZones) {
    std::cout << *zoneIt << std::endl;
  }
  std::cout << "nums zones are:" << std::endl;
  for (const auto& zoneIt : _numZones) {
    std::cout << *zoneIt << std::endl;
  }
  std::cout << "trash zones are:" << std::endl;
  for (const auto& zoneIt : _trashZones) {
    std::cout << *zoneIt << std::endl;
  }
  std::cout << "raw data path is '" <<  _rawDataPath << "'" << std::endl;
  std::cout << "index data path is '" <<  _indexDataPath << "'" << std::endl;
  std::cout << "plain index file name is '" <<  _plainIndexFile << "'" << std::endl;
  std::cout << "invert index file name is '" <<  _invertIndexFile << "'" << std::endl;
}


void Indexer::parseRawData() {
  if (!boost::filesystem::exists(_rawDataPath)) {
    throw Exception("Directory '" + _rawDataPath + "' doesn't exist");
  }
  else if (!boost::filesystem::is_directory(_rawDataPath)) {
    throw Exception("'" + _rawDataPath + "' isn't a directory");
  }
  //
  for (boost::filesystem::directory_iterator dir_it(_rawDataPath); dir_it != boost::filesystem::directory_iterator(); ++dir_it) {
    auto filePath = dir_it->path().string();
    if (!boost::filesystem::is_regular_file(dir_it->status())) {
      throw Exception("'" + filePath + "' isn't a regular file");
    }
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filePath.c_str())) {
      throw Exception("Cannot load xml file '" + filePath + "'");
    }
    try {
      auto itemsTag = getNecessaryTag(*doc.ToElement(), "items");
      for (auto itemTag = itemsTag->FirstChildElement(); itemTag != 0 ; itemTag = itemTag->NextSiblingElement()) {
        if (std::strcmp(itemTag->Name(), "item")) {
          std::string name = itemTag->Name();
          throw Exception("There is an incorrect tag '" + name + "'");
        }
        extractTextZones(itemTag);
        extractNumZones(itemTag);
        //..
      }
    }
    catch (Indexer::Exception& ie) {
      throw Exception("Parse file '" + filePath + "' is failed. " + ie.errMsg)
    }
  }
}


void Indexer::extractTextZones(const tinyxml2::XMLElement* tiElem) {
  for (auto& zone : _textZones) {
    const tinyxml2::XMLElement* curTag = tiElem;
    for (auto& tag : zone->path) {
      curTag = curTag->FirstChildElement(tag.c_str());
    }
    if (!curTag && !zone->isOptional) {
      throw Exception("Zone '" + zone->name + "' is necessary");
    }
    //...
  }
}


void Indexer::extractNumZones(const tinyxml2::XMLElement* tiElem) {
  return;
}



int main(int argc, char *argv[])
{
  const char *configPath;
  if (argc == 2) {
    configPath = argv[1];
  }
  else {
    std::cout << "Usage: '" << argv[0] << "' path_to_config" << std::endl;
    return 1;
  }
  try {
    Indexer indexer(configPath);
    indexer.tmpPrint();
    indexer.parseRawData();
  }
  catch (std::exception& e) {
    std::cout << e.what() << std::endl;
  }
  //
  return 0;
}
