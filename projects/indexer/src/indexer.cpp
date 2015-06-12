#include <sstream>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "indexer.h"
#include "memory_control.h"
#include "tokenizer.hpp"
#include "normalizer.hpp"
#include "porter2_stemmer.h"
#include "loc.hpp"



uint32_t stou(const std::string& str) {
  uint32_t res = 0;
  if (!str.empty()) {
    std::istringstream iss(str);
    iss >> res;
  }
  return res;
}



///////////////////////////////////////////////////////////////////////////////////////////



void DynDocStorage::save(const std::string& dataPath) {
  std::cout << "Saving document storage..." << std::endl;
  std::ofstream ofs(dataPath, std::ofstream::out | std::ofstream::binary);
  //
  uint32_t count = _docIds.size();
  ofs << static_cast<uint8_t>(_nZones.size());
  ofs.write(reinterpret_cast<char*>(&_docIds[0]), count*sizeof(uint32_t));
  for (auto& vec: _nZones) {
    ofs.write(reinterpret_cast<char*>(&vec[0]), count*sizeof(uint32_t));
  }
  for (auto& vec: _tZonesWCnt) {
    ofs.write(reinterpret_cast<char*>(&vec[0]), count*sizeof(uint16_t));
  }
}




void DynDocStorage::load(const std::string& dataPath) {
  std::cout << "Loading document storage..." << std::endl;
  try {
    _load(dataPath);
  }
  catch (Index::Exception& e) {
    std::cout << "There is no document storage. Create one." << std::endl;
  }
}



void DynDocStorage::addDoc(uint32_t docId, const std::vector<uint32_t>& nZones, const std::vector<uint16_t>& tZonesWCnt) {
  if (nZones.size() != _nZones.size() || tZonesWCnt.size() != _tZonesWCnt.size()) {
    throw Index::Exception("Cannot add new document to DynDocStorage, there is incorrect count of numeric zones");
  }
  _docIds.push_back(docId);
  for (size_t i = 0; i < _nZones.size(); ++i) {
    _nZones[i].push_back(nZones[i]);
  }
  for (size_t i = 0; i < _tZonesWCnt.size(); ++i) {
    _tZonesWCnt[i].push_back(tZonesWCnt[i]);
  }
}



size_t DynDocStorage::size() {
  return _docIds.size();
}



///////////////////////////////////////////////////////////////////////////////////////////



uint32_t DynPostingStore::addTokenPosting(const std::vector< std::vector<uint16_t> >& zonesPosting) {
  for (const auto& vec: zonesPosting) {
    _postingStore.push_back(vec.size());
    std::copy(vec.begin(), vec.end(), std::back_inserter(_postingStore));
  }
  return _postingStore.size();
}



size_t DynPostingStore::size() {
  return _postingStore.size();
}



std::ofstream& operator<<(std::ofstream& ofs, DynPostingStore& dps) {
  ofs << static_cast<uint32_t>(dps._postingStore.size());
  ofs.write(reinterpret_cast<char*>(&dps._postingStore[0]), dps._postingStore.size()*sizeof(uint16_t));
  return ofs;
}



///////////////////////////////////////////////////////////////////////////////////////////



Indexer::Indexer(const Index::Config& config, uint64_t maxMemoryUsage) :
  _maxMemSize(maxMemoryUsage), _charsCnt(0), _entriesCnt(0),
  _config(config), _docStore(config.numZones.size(), config.textZones.size())
{ }



void Indexer::cook() {
  _docStore.load(_config.indexDataPath + '/' + _config.docStoreFile);
  _parseRawData();
  //_mergeIndexes();
  _docStore.save(_config.indexDataPath + '/' + _config.docStoreFile);
}



template <typename K, typename V>
size_t estimateMapSize(const std::map<K,V>& map) {
  return (sizeof(K) + sizeof(V) + sizeof(std::_Rb_tree_node_base)) * map.size() + sizeof(map);
}



template <typename K, typename V>
uint64_t estimateUMapSize(const std::unordered_map<K,V>& map) {
  uint32_t bc = map.bucket_count() * (sizeof(K) + sizeof(V));
  double mlf = map.max_load_factor();
  return (mlf > 1.0) ?  bc * mlf : bc;
}



bool Indexer::_hasSpace() {
  uint64_t curSize = estimateMapSize(_invIdx)
    + estimateUMapSize(_statDict);
    + _entriesCnt * sizeof(Index::Entry)
    + _charsCnt * sizeof(char)
    + _postingStore.size() * sizeof(uint16_t);
  return curSize < _maxMemSize;
}



void Indexer::_parseRawData() {
  uint32_t itemsCnt = 0;
  uint32_t checkInterval = 1000;
  if (!boost::filesystem::exists(_config.rawDataPath)) {
    throw Index::Exception("Directory '" + _config.rawDataPath + "' doesn't exist");
  }
  else if (!boost::filesystem::is_directory(_config.rawDataPath)) {
    throw Index::Exception("'" + _config.rawDataPath + "' isn't a directory");
  }
  //
  std::vector<uint16_t> wordsCnt(_config.textZones.size());
  for (boost::filesystem::directory_iterator dir_it(_config.rawDataPath); dir_it != boost::filesystem::directory_iterator(); ++dir_it) {
    auto filePath = dir_it->path().string();
    if (!boost::filesystem::is_regular_file(dir_it->status())) {
      throw Index::Exception("'" + filePath + "' isn't a regular file");
    }
    std::cout << "Parsing file '" << filePath << "'..." << std::endl;
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filePath.c_str())) {
      throw Index::Exception("Cannot load xml file '" + filePath + "'");
    }
    try {
      if (itemsCnt % checkInterval == checkInterval - 1 && !_hasSpace()) {
        _flushToDisk();
      }
      auto itemsTag = getNecessaryTag(&doc, "items");
      for (auto itemTag = itemsTag->FirstChildElement(); itemTag != 0 ; itemTag = itemTag->NextSiblingElement()) {
        if (std::strcmp(itemTag->Name(), "item")) {
          std::string name = itemTag->Name();
          throw Index::Exception("There is an incorrect tag '" + name + "'");
        }
        auto docId = stou(getNecessaryTag(itemTag, _config.docIdTag)->GetText());
        std::fill(wordsCnt.begin(), wordsCnt.end(), 0);
        _extractTextZones(itemTag, wordsCnt);
        _extractNumZones(itemTag, docId, wordsCnt);
      }
    }
    catch (Index::Exception& ie) {
      throw Index::Exception("Parse file '" + filePath + "' is failed. " + ie.errMsg);
    }
  }
}



void extractZones(const tinyxml2::XMLElement* tiElem, const std::vector<Index::Zone*>& zonesVec, std::vector<std::string>& outVec) {
  outVec.resize(zonesVec.size(), "");
  for (size_t zi = 0; zi < zonesVec.size(); ++zi) {
    const tinyxml2::XMLElement* curTag = tiElem;
    for (auto& tag : zonesVec[zi]->path) {
      curTag = curTag->FirstChildElement(tag.c_str());
    }
    if (!curTag && !zonesVec[zi]->isOptional) {
      throw Index::Exception("Zone '" + zonesVec[zi]->name + "' is necessary");
    }
    else if (curTag && curTag->GetText()) {
      outVec[zi] = curTag->GetText();
    }
  }
}



void splitByZones(const std::vector<uint16_t>& posting, const std::vector<uint16_t>& zwCnt, std::vector< std::vector<uint16_t> >& splitPosting, std::vector<bool>& inZone) {
  uint8_t zi = 0;
  uint16_t offset = 0;
  bool isNewZone = true;
  for (const auto& val : posting) {
    while (val >= offset + zwCnt[zi]) {
      offset += zwCnt[zi++];
      isNewZone = true;
    }
    if (isNewZone) {
      splitPosting.push_back(std::vector<uint16_t>());
      inZone[zi] = true;
    }
    splitPosting[zi].push_back(val - offset);
  }
}



void Indexer::_addToStatDict(const std::string& word) {
  auto it = _statDict.find(word);
  if (it != _statDict.end()) {
    _charsCnt += word.size();
    _statDict.emplace(word, 0);
  }
  else {
    ++it->second;
  }
}



void Indexer::_extractTextZones(const tinyxml2::XMLElement* tiElem, std::vector<uint16_t>& wordsCnt) {
  uint16_t pos = 0;
  std::vector<std::string> bufVec;
  std::vector<std::string> zonesVec;
  extractZones(tiElem, _config.textZones, zonesVec);
  std::unordered_map<uint32_t,std::vector<uint16_t>> tmpDict;
  for (size_t zi = 0; zi < zonesVec.size(); ++zi) {
    Common::terminate2vec(zonesVec[zi], bufVec);
    for (auto& word : bufVec) {
      _addToStatDict(word);
      ++wordsCnt[zi];
      auto tokId = MurmurHash2(word.data(), word.size());
      tmpDict[tokId].push_back(pos++);
    }
    bufVec.clear();
  }
  uint32_t posOfs, docOfs;
  std::vector< std::vector<uint16_t> > fzpBuf;
  std::vector<bool> inZone(_config.textZones.size());
  for (const auto& pair : tmpDict) {
    splitByZones(pair.second, wordsCnt, fzpBuf, inZone);
    posOfs = _postingStore.addTokenPosting(fzpBuf);
    docOfs = _docStore.size();
    _invIdx[pair.first].push_back(Index::Entry(docOfs, posOfs, inZone));
    std::fill(inZone.begin(), inZone.end(), true);
    fzpBuf.clear();
  }
  _entriesCnt += tmpDict.size();
}



void Indexer::_extractNumZones(const tinyxml2::XMLElement* tiElem, uint32_t docId, std::vector<uint16_t>& wordsCnt) {
  std::vector<std::string> zonesVec;
  extractZones(tiElem, _config.numZones, zonesVec);
  std::vector<uint32_t> zonesVal;
  for (size_t zi = 0; zi < zonesVec.size(); ++zi) {
    zonesVal.push_back(stou(zonesVec[zi]));
  }
  _docStore.addDoc(docId, zonesVal, wordsCnt);
}



std::string Indexer::_findFirstEmptyFile()
{
  if (!boost::filesystem::exists(_config.indexDataPath)) {
    throw Index::Exception("Directory '" + _config.indexDataPath + "' doesn't exist");
  }
  else if (!boost::filesystem::is_directory(_config.indexDataPath)) {
    throw Index::Exception("'" + _config.indexDataPath + "' isn't a directory");
  }
  //
  uint32_t sfx = 0;
  std::string path;
  do {
    path = _config.indexDataPath + '/' + _config.invertIndexFile + std::to_string(sfx++);
  } while (!boost::filesystem::exists(path));
  return path;
}



void Indexer::_flushToDisk() {
  const std::string path = _findFirstEmptyFile();
  std::cout << "Flusing data to '" << path << "'..." << std::endl;
  std::ofstream ofs(path, std::ofstream::out | std::ofstream::binary);
  //
  ofs << static_cast<uint32_t>(_invIdx.size());
  for (const auto& pair: _invIdx) {
    ofs << pair.first << static_cast<uint32_t>(pair.second.size());
    for (const auto& entry: pair.second) {
      ofs << entry.docIdOffset << entry.postingOffset;
    }
  }
  ofs << _postingStore;
  ofs.close();
  //
  std::ofstream statistic_ofs(_config.indexDataPath + "/words_frequency.draw", std::ofstream::app);
  for (const auto& pair: _statDict) {
    ofs << pair.first << " " << pair.second << std::endl;
  }
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
    Common::init_locale("en_US.UTF-8");
    Index::Config config(configPath);
    Indexer indexer(config);
    indexer.cook();
  }
  catch (std::exception& e) {
    std::cerr << "Catch the exception: " << e.what() << std::endl;
  }
  //
  return 0;
}
