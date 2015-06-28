#include "index_io.hpp"
#include "../../../include/indexer/index_struct.h"
#include "tinyxml2_ext.hpp"
#include "MurmurHash2.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>



using namespace Index;

const double EPSILON = std::numeric_limits<double>::epsilon();



///////////////////////////////////////////////////////////////////////////////////////////



bool parseBoolTag(const tinyxml2::XMLElement* tag) {
  if (tag) {
    std::string text = tag->GetText();
    if (text == "true" || text == "True" || text == "1") {
      return true;
    }
    else if (text == "false" || text == "False" || text == "0") {
      return false;
    }
    else {
      throw Exception("Cannot load config. Zone \"optional\" must be \"[Tt]rue|[Ff]alse|[01]\"");
    }
  }
  return false;
}



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
  zone.isOptional = parseBoolTag(tag);
  tag = tiElem->FirstChildElement("needSave");
  zone.needSave = parseBoolTag(tag);
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
      Posting np(_ifs, po * sizeof(uint16_t), _postingSizes[pso]);
      po += _postingSizes[pso++];
      res[i] = np;
    }
  }
  return res;
}



void PostingStorage::_load(const std::string& path) {
  _ifs.open(path, std::ios::in | std::ios::binary);
  if (!_ifs.is_open()) { 
    throw Index::Exception("Cannot open '" + path + "'");
  }
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
    throw Exception("Cannot open '" + dataPath + "'");
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




void InvIdxItem::read(std::ifstream& ifs) {
  readFrom(ifs, &tokId);
  ifs >> entries;
}



bool compareInvIdxItem(const InvIdxItem& li, const InvIdxItem& ri) {
  return li.tokId < ri.tokId;
}



//////////////////////////////////////////////////////////////////////////////////////////



//InvertIndex::InvertIndex(const Config& config) :
//  _config(config), _postingStore(config), _docStore(config)
//{
//  std::ifstream ifs(_config.indexDataPath + _config.invertIndexFile, std::ios::in | std::ios::binary);
//  uint32_t size = readFromEnd(ifs);
//  _invIdx.resize(size);
//  for (auto item: _invIdx) {
//    item.read(ifs);
//  }
//  _idf.resize(_config.textZones.size());
//  for (auto vec: _idf) {
//    vec.resize(_invIdx.size());
//  }
//}



void InvertIndex::configure(const std::string& pathToConfig) {
  _pConfig = new Config(pathToConfig);
  _pPostingStore = new PostingStorage(*_pConfig);
  _pDocStore = new DocStorage(*_pConfig);
  _docDB.open(_pConfig->indexDataPath + _pConfig->documentDb);
  std::ifstream ifs(_pConfig->indexDataPath + _pConfig->invertIndexFile, std::ios::in | std::ios::binary);
  uint32_t size = readFromEnd(ifs);
  _invIdx.resize(size);
  for (auto& item: _invIdx) {
    item.read(ifs);
  }
  //_idf.resize(_config.textZones.size());
  //for (auto vec: _idf) {
  //  vec.resize(_invIdx.size());
  //}
}



InvertIndex::~InvertIndex() {
  if (!_pConfig) delete _pConfig;
  if (!_pPostingStore) delete _pPostingStore;
  if (!_pDocStore) delete _pDocStore;
}




uint32_t InvertIndex::findTknIdx(const std::string& word) {
  InvIdxItem tmp;
  tmp.tokId = MurmurHash2(word.data(), word.size());
  return std::lower_bound(_invIdx.begin(), _invIdx.end(), tmp, compareInvIdxItem) - _invIdx.begin();
}



std::vector<Entry> InvertIndex::getEntries(uint32_t tokId) {
  return _invIdx[tokId].entries;
}




uint32_t InvertIndex::getDocId(const Entry& entry) {
  return _pDocStore->getDocId(entry.docIdOffset);
}



uint32_t InvertIndex::getNZone(const Entry& entry, uint8_t nZoneId) {
  return _pDocStore->getNumZone(entry.docIdOffset, nZoneId);
}



double InvertIndex::getTF(const Entry& entry, uint8_t tZoneId) {
  uint8_t offset = 0;
  for (size_t i = 0; i < tZoneId; ++i) {
    if (i2mask[i] & entry.inZone) {
      ++offset;
    }
  }
  double postingSize = _pPostingStore->getPostingSize(entry.postingOffset + offset);
  double zoneSize = _pDocStore->getTZoneWCnt(entry.docIdOffset, tZoneId);
  return postingSize / zoneSize;
}



std::vector<Posting> InvertIndex::getFullPosting(const Entry& entry) {
  return _pPostingStore->getFullPosting(entry);
}



//double InvertIndex::getIDF(uint32_t tknIdx, uint8_t tZoneId) {
//  return _idf[tZoneId][tknIdx];
//}



RawDoc* InvertIndex::getRawDoc(uint32_t docId) {
  return _docDB.getDoc(docId);
}



//////////////////////////////////////////////////////////////////////////////////////////



class NoMoreElements { };
inline size_t jump(std::vector<Entry>& vec, Entry& entry, size_t pos, size_t step) {
  size_t next_pos = std::min(pos + step, vec.size() - 1);
  while(next_pos < vec.size() && vec[next_pos].docIdOffset <= entry.docIdOffset) {
    pos = next_pos;
    next_pos += step;
  }
  next_pos = std::min(next_pos, vec.size() - 1);
  if (vec[next_pos].docIdOffset >= entry.docIdOffset) {
    while (pos < next_pos && vec[pos].docIdOffset < entry.docIdOffset) {
      ++pos;
    }
  }
  if (vec[pos].docIdOffset < entry.docIdOffset) throw NoMoreElements();
  return pos;
}



//inline void convertToEntries(std::vector< std::vector<Entry> >& res, std::vector<Entry>& entries) {
//  entries.resize(res.size());
//  for (int i = 0; i < res.size(); ++i) {
//    entries[i] = res[i].front();
//  }
//}



void intersectMore(std::vector< std::vector<Entry> >& common, std::vector<Entry>& entries, std::vector< std::vector<Entry> >& res, size_t step) {
  size_t i1, i2;
  i1 = i2 = 0;
  std::vector<Entry> v1(common.size());
  for (size_t i = 0; i < common.size(); ++i) {
    v1[i] = common[i].front();
  }
  std::vector<Entry>& v2 = entries;
  if (v1.empty() || v2.empty()) return;
  try {
    while (42) {
      if (v1[i1].docIdOffset < v2[i2].docIdOffset) {
        i1 = jump(v1, v2[i2], i1, step);
      }
      else if (v1[i1].docIdOffset > v2[i2].docIdOffset) {
        i2 = jump(v2, v1[i1], i2, step);
      }
      else {
        res.push_back(std::vector<Entry>());
        std::copy(common[i1].begin(), common[i1].end(), std::back_inserter(res.back()));
        res.back().push_back(v2[i2]);
        //res.push_back(v1[i1]);
        ++i1; ++i2;
        if (i1 == v1.size() || i2 == v2.size()) return;
      }
    }
  }
  catch (NoMoreElements) { return; }
}



bool compareSizes(const std::vector<Entry>& v1, const std::vector<Entry>& v2) {
  return v1.size() < v2.size();
}



void Index::intersectEntries(std::vector< std::vector<Entry> >& input, std::vector< std::vector<Entry> >& output) {
  if (input.empty()) return;
  size_t step = 100;
  std::sort(input.begin(), input.end(), compareSizes);
  output.push_back(std::vector<Entry>());
  output.resize(input.front().size());
  for (size_t i = 0; i < input.front().size(); ++i) {
    output[i].push_back(input.front()[i]);
  }
  //
  std::vector< std::vector<Entry> > tmp;
  for (size_t i = 1; i < input.size(); ++i) {
    if (i % 2 == 1) {
      tmp.clear();
      intersectMore(output, input[i], tmp, step);
    }
    else if (i % 2 == 0) {
      output.clear();
      intersectMore(tmp, input[i], output, step);
    }
  }
  if (input.size() % 2 == 0) {
    output = tmp;
  }
}




//////////////////////////////////////////////////////////////////////////////////////////



DocDatabase::DocDatabase() : _db(0) { }



DocDatabase::~DocDatabase() {
  if (_db != 0) delete _db;
}



void DocDatabase::open(const std::string& path) {
  leveldb::Options options;
  options.create_if_missing = true;
  //
  leveldb::Status status = leveldb::DB::Open(options, path, &_db);
  if (!status.ok()) {
    throw Exception("Unable to open/create test database '" + path + "'. Reason: " + status.ToString());
  }
}



void readStr(std::istream& is, std::string& str) {
  uint32_t size;
  readFrom(is, &size);
  str.resize(size);
  is.read(reinterpret_cast<char*>(&str[0]), str.size()*sizeof(char));
}



void writeStr(std::ostream& os, std::string& str) {
  writeTo(os, str.size());
  os.write(reinterpret_cast<char*>(&str[0]), str.size()*sizeof(char));
}



RawDoc* DocDatabase::getDoc(uint32_t key) {
  std::string value;
  leveldb::Status status = _db->Get(leveldb::ReadOptions(), std::to_string(key), &value);
  std::string title, enSub, time;
  std::istringstream iss(value);
  //
  RawDoc* doc = new RawDoc();
  readStr(iss, doc->videoId);
  readStr(iss, doc->title);
  readStr(iss, doc->subtitles);
  readStr(iss, doc->time);
  //
  return doc;
}




void DocDatabase::putDoc(uint32_t key, RawDoc& doc) {
  std::ostringstream oss;
  writeStr(oss, doc.videoId);
  writeStr(oss, doc.title);
  writeStr(oss, doc.subtitles);
  writeStr(oss, doc.time);
  leveldb::Status status = _db->Put(leveldb::WriteOptions(), std::to_string(key), oss.str());
}




//////////////////////////////////////////////////////////////////////////////////////////



RawDoc::RawDoc(std::map<std::string,std::string>& zones) {
  title = zones["title"];
  subtitles = zones["en_sub"];
  time = zones["en_sub_time"];
  videoId = zones["video_id"];
}
