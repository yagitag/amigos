#include "index_io.hpp"
#include "../../../include/indexer/index_struct.h"
#include "tinyxml2_ext.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>



using namespace Index;

const double EPSILON = std::numeric_limits<double>::epsilon();



///////////////////////////////////////////////////////////////////////////////////////////



void parseZone(Zone& zone, const tinyxml2::XMLElement* tiElem) {
  zone.name = getNecessaryTag(tiElem, "name")->GetText();
  //
  auto tag = tiElem->FirstChildElement("path");
  if (tag) {
    std::istringstream iss(tag->GetText());
    std::string pathTag;
    while(std::getline(iss, pathTag, ',')) {
      zone.path.push_back(pathTag);
    }
  }
  else {
    zone.path.push_back(zone.name);
  }
  //
  tag = tiElem->FirstChildElement("optional");
  if (tag) {
    std::string text = tag->GetText();
    if (text == "true" || text == "True" || text == "1") {
      zone.isOptional = true;
    }
    else if (text == "false" || text == "False" || text == "0") {
      zone.isOptional = false;
    }
    else {
      throw Exception("Cannot load config. Zone \"optional\" must be \"[Tt]rue|[Ff]alse|[01]\"");
    }
  }
  else {
    zone.isOptional = false;
  }
}



Config::Config(const std::string& configPath) {
  tinyxml2::XMLDocument doc;
  if (doc.LoadFile(configPath.c_str())) {
    throw Exception("Cannot load xml file '" + configPath + "'");
  }
  for (auto tag = doc.FirstChildElement(); tag != 0 ; tag = tag->NextSiblingElement()) {
    std::string tagName = tag->Name();
    if (tagName == "doc_id") {
      docIdTag = tag->GetText();
    }
    else if (tagName == "raw_data") {
      rawDataPath = getNecessaryTag(tag, "path")->GetText();
    }
    else if (tagName == "index_data") {
      indexDataPath = getNecessaryTag(tag, "path")->GetText();
      invertIndexFile = getNecessaryTag(tag, "invert_index")->GetText();
      docStoreFile = getNecessaryTag(tag, "document_info")->GetText();
      documentDb = getNecessaryTag(tag, "document_database")->GetText();
      postingsFile = getNecessaryTag(tag, "postings")->GetText();
    }
    else {
      std::vector<Zone*>* pZones;
      if (tagName == "text_zones") {
        pZones = &textZones;
      }
      else if (tagName == "num_zones") {
        pZones = &numZones;
      }
      else if (tagName == "trash_zones") {
        pZones = &trashZones;
      }
      else {
        throw Exception("Failed to load config. Unsupported tag '" + tagName + "'");
      }
      for (auto zoneTag = tag->FirstChildElement(); zoneTag != 0 ; zoneTag = zoneTag->NextSiblingElement()) {
        auto zone = new Zone;
        zone->id = pZones->size();
        parseZone(*zone, zoneTag);
        pZones->push_back(zone);
      }
    }
  }
}



Config::~Config() {
  for (auto& zone : textZones) {
    delete zone;
  }
  for (auto& zone : numZones) {
    delete zone;
  }
  for (auto& zone : trashZones) {
    delete zone;
  }
}



//////////////////////////////////////////////////////////////////////////////////////////


Posting::Posting(std::istream& is, uint32_t seek, uint32_t size) :
  _data(new uint16_t[size], std::default_delete<uint16_t[]>())
{
  is.seekg(seek, is.beg);
  //_data.get()[2] = 3;
  is.read(reinterpret_cast<char*>(_data.get()), size*sizeof(uint16_t));
  begin = _data.get();
  end = begin + size;
}


//////////////////////////////////////////////////////////////////////////////////////////



uint16_t PostingStorage::getPostingSize(uint32_t offset) const {
  return _postingSizes[offset];
}



std::vector<Posting> PostingStorage::getFullPosting(const Entry& entry) {
  std::vector<Posting> res(_tZonesCnt);
  uint32_t po = entry.postingOffset;
  uint32_t pso = entry.postingsSizeOffset;
  for (size_t i = 0; i < _tZonesCnt; ++i) {
    if (entry.inZone & i2mask[i]) {
      //Posting np(&_postingStore[po], &_postingStore[po] + _postingSizes[pso]);
      Posting np(_ifs, po, _postingSizes[pso]);
      po += _postingSizes[pso++];
      res[i] = np;
    }
  }
  return res;
}



void PostingStorage::_load(const std::string& path) {
  _ifs.open(path, std::ios::in | std::ios::binary);
  _commonSize = readFromEnd(_ifs);
  _ifs.seekg(_commonSize * sizeof(uint16_t), _ifs.beg);
  _ifs >> _postingSizes;
  //_ifs.close();
  //_ofs.open(path, std::ios::out | std::ios::binary);
  //_ifs.read(reinterpret_cast<char*>(&_postingSizes[0]), size*sizeof(uint32_t));
}



//std::istream& operator>>(std::istream& ifs, Index::PostingStorage& ps) {
//  ifs >> ps._postingStore;
//  return ifs;
//}



///////////////////////////////////////////////////////////////////////////////////////////



void DocStorage::_load(const std::string& dataPath, bool isAppendMode) {
  std::ifstream ifs(dataPath, std::ios::in | std::ifstream::binary);
  if (!ifs.is_open()) {
    throw Exception("Cannot open document storage");
  }
  uint32_t nZonesCnt, tZonesCnt;
  readFrom(ifs, &nZonesCnt);
  readFrom(ifs, &tZonesCnt);
  uint32_t start = 0, count = 0;
  if ( isAppendMode) {
    if (_nZones.size() != nZonesCnt) {
      throw Exception("Cannot load from '" + dataPath + "' into index: count of numeric zones isn't equal");
    }
    else if (_tZonesWCnt.size() != tZonesCnt) {
      throw Exception("Cannot load from '" + dataPath + "' into index: count of text zones isn't equal");
    }
    start = _docIds.size();
  }
  else {
    _nZones.resize(nZonesCnt);
    _tZonesWCnt.resize(tZonesCnt);
  }
  //
  readFrom(ifs, &count);
  _docIds.resize(start + count);
  ifs.read(reinterpret_cast<char*>(&_docIds[start]), count*sizeof(uint32_t));
  for (auto& vec: _nZones) {
    vec.resize(start + count);
    ifs.read(reinterpret_cast<char*>(&vec[start]), count*sizeof(uint32_t));
  }
  for (auto& vec: _tZonesWCnt) {
    vec.resize(start + count);
    ifs.read(reinterpret_cast<char*>(&vec[start]), count*sizeof(uint16_t));
  }
}



//////////////////////////////////////////////////////////////////////////////////////////



InvertIndex::InvertIndex(const Config& config) :
  _postingStore(config), _docStore(config)
{
  
}
