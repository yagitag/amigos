#include <list>
#include <sstream>
#include <fstream>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "loc.hpp"
#include "../include/indexer.h"
#include "../include/index_io.hpp"
#include "tokenizer.hpp"
#include "normalizer.hpp"
#include "porter2_stemmer.h"
#include "../../../contrib/leveldb/include/leveldb/db.h"


uint32_t stou(const std::string& str) {
  uint32_t res = 0;
  if (!str.empty()) {
    std::istringstream iss(str);
    iss >> res;
  }
  return res;
}



//////////////////////////////////////////////////////////////////////////////////////////
//
//
//EntriesComparator::EntriesComparator(const DocStorage& ds, const PostingStorage& ps) :
//  _ds(ds), _ps(ps) { }
//
//
//bool EntriesComparator::operator()(const Entry& l, const Entry& r) {
//  uint8_t nStep = 0;
//  for (size_t i = 0; i < l.inZone.size(); ++i) {
//    if (l.inZone[i] < r.inZone[i])      return true;
//    else if (l.inZone[i] > r.inZone[i]) return false;
//    else if (l.inZone[i] == true ) {
//      ++nStep;
//      double ltf = _ps.getPostingSize(l.postingOffset, nStep) / _ds.getTZoneWCnt(l.docIdOffset, i);
//      double rtf = _ps.getPostingSize(r.postingOffset, nStep) / _ds.getTZoneWCnt(r.docIdOffset, i);
//      if (std::fabs(ltf - rtf) < EPSILON) continue;
//      return ltf < rtf;
//    }
//  }
//  return false;
//}
//
//
//
bool compareEntries(const Index::Entry& e1, const Index::Entry& e2) {
  return e1.docIdOffset < e2.docIdOffset;
}
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////



DynDocStorage::DynDocStorage(const Index::Config& config) :
  _path(config.indexDataPath + config.docStoreFile) 
{
  _nZones.resize(config.numZones.size());
  _tZonesWCnt.resize(config.textZones.size());
  std::cout << "Loading document storage..." << std::endl;
  try {
    _load(_path);
  }
  catch (Index::Exception& e) {
    std::cout << "Catch the exception '" << e.what() << "' while loading document storage!" << std::endl;
    std::cout << "Creating a new one." << std::endl;
  }
}



DynDocStorage::~DynDocStorage() {
  std::cout << "Saving document storage..." << std::endl;
  std::ofstream ofs(_path, std::ofstream::out | std::ofstream::binary);
  //
  uint32_t count = _docIds.size();
  writeTo(ofs, static_cast<uint32_t>(_nZones.size()));
  writeTo(ofs, static_cast<uint32_t>(_tZonesWCnt.size()));
  writeTo(ofs, count);
  ofs.write(reinterpret_cast<char*>(&_docIds[0]), count*sizeof(uint32_t));
  for (auto& vec: _nZones) {
    ofs.write(reinterpret_cast<char*>(&vec[0]), count*sizeof(uint32_t));
  }
  for (auto& vec: _tZonesWCnt) {
    ofs.write(reinterpret_cast<char*>(&vec[0]), count*sizeof(uint16_t));
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



DynPostingStore::DynPostingStore(const Index::Config& config) :
  _path(config.indexDataPath + config.postingsFile)
{
  _commonSize = 0;
  _tZonesCnt = config.textZones.size();
  std::cout << "Init posting storage..." << std::endl;
  try {
    _load(_path);
  }
  catch (Index::Exception& e) {
    std::cout << "Catch the exception '" << e.what() << "' while loading posting storage!" << std::endl;
    std::cout << "Creating a new one." << std::endl;
  }
  _ifs.close();
  _ofs.open(_path, std::ios::app | std::ios::binary);
}



DynPostingStore::~DynPostingStore() {
  std::cout << "Deinit posting storage..." << std::endl;
  std::cout << _ofs.tellp() << std::endl;
}



void DynPostingStore::addTokenPosting(std::vector< std::vector<uint16_t> >& zonesPosting, uint64_t* postingOffset) {
  *postingOffset = _commonSize;
  uint16_t size;
  for (auto& vec: zonesPosting) {
    _commonSize += (vec.size() + 1);
    size = vec.size();
    writeTo(_ofs, size);
    _ofs.write(reinterpret_cast<char*>(&vec[0]), vec.size()*sizeof(uint16_t));
  }
}



//size_t DynPostingStore::size() {
//  return _postingStore.size() + _postingSizes.size();
//}
//
//
//
//void DynPostingStore::clear() {
//  _postingStore.clear();
//  _postingSizes.clear();
//}
//
//
//
//std::ostream& operator<<(std::ostream& ofs, DynPostingStore& dps) {
//  ofs << dps._postingStore;
//  ofs << dps._postingSizes;
//  return ofs;
//}



///////////////////////////////////////////////////////////////////////////////////////////



Indexer::Indexer(const Index::Config& config, uint64_t maxMemoryUsage) :
  _maxMemSize(maxMemoryUsage), _charsCnt(0), _entriesCnt(0),
  _config(config), _docStore(config), _postingStore(config)
{
  _docDB.open(_config.indexDataPath + _config.documentDb);
  _bigramer.configure(_config.confDataPath + _config.bigramerPath);
}


Indexer::~Indexer() { }



void Indexer::cook() {
  try {
    _parseRawData();
    _flushToDisk();
    _mergeIndexes();
  }
  catch(Index::Exception& e) {
    _flushToDisk();
    throw e;
  }
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
    + estimateUMapSize(_statDict)
    + _entriesCnt * (sizeof(Index::Entry) + sizeof(double) * _config.textZones.size())
    + _charsCnt * sizeof(char);
    //+ _postingStore.size() * sizeof(uint16_t);
  return curSize < _maxMemSize;
}



void extractZones(const tinyxml2::XMLElement* tiElem, const std::vector<Index::Zone*>& zonesVec, std::vector<std::string>& outVec, std::map<std::string,std::string>& forSave) {
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
      if (zonesVec[zi]->needSave) {
        forSave[zonesVec[zi]->name] = outVec[zi];
      }
    }
  }
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
  std::vector<uint32_t> numZones(_config.numZones.size());
  std::map<std::string,std::string> zonesForSave;
  std::vector<std::string> fake;
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
      auto itemsTag = getNecessaryTag(&doc, "items");
      for (auto itemTag = itemsTag->FirstChildElement(); itemTag != 0 ; itemTag = itemTag->NextSiblingElement()) {
        if (itemsCnt++ % checkInterval == checkInterval - 1 && !_hasSpace()) {
          _flushToDisk();
        }
        if (std::strcmp(itemTag->Name(), "item")) {
          std::string name = itemTag->Name();
          throw Index::Exception("There is an incorrect tag '" + name + "'");
        }
        auto docId = stou(getNecessaryTag(itemTag, _config.docIdTag)->GetText());
        //
        std::fill(wordsCnt.begin(), wordsCnt.end(), 0);
        //std::fill(numZones.begin(), numZones.end(), 0); нет смысла, они все заполняются
        //
        //TODO проверку на наличие дубля
        _extractNumZones(itemTag, numZones, zonesForSave);
        if (numZones[3] == 1)  continue; //если английские сабы переведены, пропускаем их
        _extractTextZones(itemTag, wordsCnt, zonesForSave);
        extractZones(itemTag, _config.trashZones, fake, zonesForSave);
        //
        _docStore.addDoc(docId, numZones, wordsCnt);
        Index::RawDoc doc(zonesForSave);
        _docDB.putDoc(docId, doc);
        for (auto pair: zonesForSave) pair.second = "";
      }
    }
    catch (Index::Exception& ie) {
      throw Index::Exception("Parse file '" + filePath + "' is failed. " + ie.errMsg);
    }
  }
}



void splitByZones(const std::vector<uint16_t>& posting, const std::vector<uint16_t>& zwCnt, std::vector< std::vector<uint16_t> >& splitPosting, std::vector<double>& zoneTf) {
  uint8_t zi = 0, inZone = 0;
  uint16_t offset = 0;
  bool isNewZone = true;
  for (const auto& val : posting) {
    while (val >= offset + zwCnt[zi]) {
      offset += zwCnt[zi++];
      isNewZone = true;
    }
    if (isNewZone) {
      splitPosting.push_back(std::vector<uint16_t>());
      inZone |= Index::i2mask[zi];
    }
    splitPosting.back().push_back(val - offset);
    isNewZone = false;
  }
  uint8_t i = 0;
  for (size_t zi = 0; zi < zwCnt.size(); ++zi) {
    if (Index::i2mask[zi] & inZone) {
      zoneTf.push_back(static_cast<double>(splitPosting[i++].size()) / zwCnt[zi]);
    } else {
      zoneTf.push_back(0.);
    }
  }
}



void Indexer::_addToStatDict(const std::string& word) {
  auto it = _statDict.find(word);
  if (it == _statDict.end()) {
    _charsCnt += word.size();
    _statDict.emplace(word, 1);
  }
  else {
    ++it->second;
  }
}



void Indexer::_extractTextZones(const tinyxml2::XMLElement* tiElem, std::vector<uint16_t>& wordsCnt, std::map<std::string,std::string>& forSave) {
  uint16_t pos = 0;
  std::vector<std::string> terms;
  std::vector<std::string> bterms;
  std::vector<std::string> zonesVec;
  extractZones(tiElem, _config.textZones, zonesVec, forSave);
  std::unordered_map< uint32_t,std::vector<uint16_t> > tmpDict;
  for (size_t zi = 0; zi < zonesVec.size(); ++zi) {
    Common::terminate2vec(zonesVec[zi], terms);
    _bigramer.bigram2vec(terms, bterms);
    for (auto& word : bterms) {
      _addToStatDict(word);
      ++wordsCnt[zi];
      auto tokId = MurmurHash2(word.data(), word.size());
      tmpDict[tokId].push_back(pos++);
    }
    bterms.clear();
    terms.clear();
  }
  std::vector< std::vector<uint16_t> > fzpBuf;
  for (const auto& pair : tmpDict) {
    // New Enty creation
    std::vector<Index::Entry>& entries = _invIdx[pair.first];
    entries.push_back(Index::Entry());
    Index::Entry& entry = entries.back();
    // New Entry initializion
    splitByZones(pair.second, wordsCnt, fzpBuf, entry.zoneTf);
    _postingStore.addTokenPosting(fzpBuf, &(entry.postingOffset));
    entry.docIdOffset = _docStore.size();
    //
    //_invIdx[pair.first].push_back(entry);
    fzpBuf.clear();
  }
  _entriesCnt += tmpDict.size();
}



void Indexer::_extractNumZones(const tinyxml2::XMLElement* tiElem, std::vector<uint32_t>& zonesVal, std::map<std::string,std::string>& forSave) {
  std::vector<std::string> zonesVec;
  extractZones(tiElem, _config.numZones, zonesVec, forSave);
  for (size_t zi = 0; zi < zonesVec.size(); ++zi) {
    zonesVal[zi] = stou(zonesVec[zi]);
  }
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
    path = _config.indexDataPath + _config.invertIndexFile + '.' + std::to_string(sfx++);
  } while (boost::filesystem::exists(path));
  return path;
}



void writeIdxItem(std::ofstream& ofs, uint32_t token, std::vector<Index::Entry>& entries) {
  writeTo(ofs, token);
  ofs << entries;
}



void Indexer::_flushToDisk() {
  const std::string path = _findFirstEmptyFile();
  std::cout << "Flusing data to '" << path << "'..." << std::endl;
  std::ofstream ofs(path, std::ofstream::out | std::ofstream::binary);
  //
  for (auto& pair: _invIdx) {
    writeIdxItem(ofs, pair.first, pair.second);
  }
  writeTo(ofs, _invIdx.size());
  _invIdx.clear();
  ofs.close();
  //
  std::ofstream statistic_ofs(_config.indexDataPath + "words_frequency.draw", std::ofstream::app);
  for (const auto& pair: _statDict) {
    statistic_ofs << pair.first << " " << pair.second << std::endl;
  }
  _statDict.clear();
  _entriesCnt = 0;
  _charsCnt = 0;
}



class RawIndex {
  public:
    RawIndex(const std::string& pathToFile) : path(pathToFile) {
      _ifs = new std::ifstream(path, std::ios::in | std::ios::binary);
      _size = readFromEnd(*_ifs);
      readNextToken();
    }
    bool readNextToken() {
      if (_size-- == 0) {
        return false;
      }
      readFrom(*_ifs, &curTok);
      return true;
    }
    void destroy() {
      delete _ifs;
      //boost::filesystem::remove(path);
    }
    void readTokEntries(std::vector<Index::Entry>& entries) {
      *_ifs >> entries;
    }

    uint32_t curTok;
    std::string path;

  private:
    std::ifstream* _ifs;
    size_t _size;
};



void Indexer::_mergeIndexes() {
  std::cout << "Merging..." << std::endl;
  std::string path = _config.indexDataPath + _config.invertIndexFile;
  std::vector<std::string> paths;
  if (boost::filesystem::exists(path)) {
    paths.push_back(path);
  }
  int sfx = 0;
  while (boost::filesystem::exists(path + '.' + std::to_string(sfx))) {
    paths.push_back(path + '.' + std::to_string(sfx));
    ++sfx;
  }
  //
  std::ofstream ofs(path + ".tmp", std::ios::out | std::ofstream::binary);
  std::list<RawIndex> indexes;
  for (const auto& path: paths) {
    indexes.push_back(RawIndex(path));
  }
  //
  std::vector<Index::Entry> entries;
  uint32_t min;
  size_t size = 0;
  while (!indexes.empty()) {
    min = std::numeric_limits<uint32_t>::max();
    for (auto& index: indexes) {
      min = std::min(min, index.curTok);
    }
    auto itIdx = indexes.begin();
    while (itIdx != indexes.end()) {
      if (itIdx->curTok == min) {
        itIdx->readTokEntries(entries);
        if (!itIdx->readNextToken()) {
          itIdx->destroy();
          itIdx = indexes.erase(itIdx);
        }
      }
      ++itIdx;
    }
    std::sort(entries.begin(), entries.end(), compareEntries);
    writeIdxItem(ofs, min, entries);
    for (auto& entry: entries) { entry.zoneTf.clear(); }
    entries.clear();
    ++size;
  }
  writeTo(ofs, size);
  ofs.close();
  boost::filesystem::rename(path + ".tmp", path);
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
  return 0;
}
