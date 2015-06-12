#ifndef __INDEX_STRUCT_H__
#define __INDEX_STRUCT_H__

#include <iostream>
#include <string>
#include <vector>

namespace Index {

  struct Zone
  {
    std::string name;
    std::vector<std::string> path;
    bool isOptional;
    int id;
  };

  struct Config
  {
    Config(const std::string& configPath);
    ~Config();
    //
    std::string docIdTag;
    //
    std::string rawDataPath;
    std::string docStoreFile;
    std::string indexDataPath;
    std::string plainIndexFile;
    std::string invertIndexFile;
    //
    std::vector<Zone*> numZones;
    std::vector<Zone*> textZones;
    std::vector<Zone*> trashZones;
  };



  struct Entry
  {
    Entry(uint32_t dOfs, uint32_t pOfs, std::vector<bool>& iz) :
      docIdOffset(dOfs), postingOffset(pOfs), inZone(iz) { }
    uint32_t docIdOffset;
    uint32_t postingOffset;
    std::vector<bool> inZone;
  };

  struct InvIdxItem
  {
    uint32_t tokId;
    std::vector<Entry> entries;
  };

  struct Posting
  {
    Posting() : begin(0), end(0) { }
    Posting(uint16_t* b, uint16_t* e) : begin(b), end(e) { }
    const uint16_t* begin;
    const uint16_t* end;
  };

  class PostingStorage 
  {
    public:
      //Posting getPosting(uint32_t offset, uint8_t tZoneId);
      std::vector<Posting> getFullPosting(const Entry& entry);
      friend std::ifstream& operator>>(std::ifstream& ifs, PostingStorage& postingStore);
    //protected:
      std::vector<uint16_t> _postingStore;
  };

  class DocStorage
  {
    public:
      DocStorage(const Config& config) {
        _load(config.indexDataPath + '/' + config.docStoreFile);
      }
      uint32_t getDocId(uint32_t docIdOfs) const {
        return _docIds[docIdOfs];
      }
      uint32_t getNumZone(uint32_t docIdOffset, uint8_t nZoneId) const {
        return _nZones[nZoneId][docIdOffset];
      }
      uint16_t getTZoneWCnt(uint32_t docIdOffset, uint8_t tZoneId) const {
        return _tZonesWCnt[tZoneId][docIdOffset];
      }
      //
    protected:
      DocStorage(uint8_t nZonesCnt, uint8_t tZonesCnt) :
        _nZones(nZonesCnt), _tZonesWCnt(tZonesCnt) { }
      void _load(const std::string& dataPath, bool isAppendMode = true);
      //
      std::vector<uint32_t> _docIds;
      std::vector<std::vector<uint32_t>> _nZones;
      std::vector<std::vector<uint16_t>> _tZonesWCnt;
  };



  //class Indexer;

  //class InvertIndex {
  //  public:
  //    Index(const std::string& configPath);
  //    const std::vector<Entry>& findToken(const std::string& word) const;
  //    uint32_t getNZone(const Entry& entry, uint8_t nZoneId); 
  //    Posting getPosting(const Entry& entry, uint8_t tZoneId);
  //    std::vector<Posting> getFullPosting(const Entry& entry);

  //    friend class Indexer;

  //  private:
  //    Index(uint8_t tZonesCnt, uint8_t nZonesCnt);
  //    void _load();
  //    void _save();
  //    void _mergeWith();

  //    std::vector<InvIdxItem> _invIdx;
  //    PostingStorage _postingStore;
  //    DocStorage _docStore;
  //};


  struct Exception : public std::exception
  {
    std::string errMsg;
    Exception(const std::string& msg) { errMsg = msg; }
    const char* what() const throw() { return errMsg.c_str(); }
  };


} // namespace Index

#endif // __INDEXSTRUCT_H__
