#ifndef __INDEX_STRUCT_H__
#define __INDEX_STRUCT_H__

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <map>

#include "../../contrib/leveldb/include/leveldb/db.h"

//namespace Index { class PostingStorage; }
//std::istream& operator>>(std::istream& ifs, Index::PostingStorage& ps);


namespace Index {

  const uint8_t i2mask[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

  struct Zone
  {
    std::string name;
    std::vector<std::string> path;
    bool isOptional;
    bool needSave;
    int id;
  };

  struct Config
  {
    Config(const std::string& configPath);
    ~Config();
    //
    std::string docIdTag;
    //
    std::string documentDb;
    std::string rawDataPath;
    std::string docStoreFile;
    std::string postingsFile;
    std::string confDataPath;
    std::string bigramerPath;
    std::string indexDataPath;
    std::string invertIndexFile;
    std::string invertIndexIPosFile; // for optimize (is optional)
    //
    std::vector<Zone*> numZones;
    std::vector<Zone*> textZones;
    std::vector<Zone*> trashZones;
  };


  struct PostingInfo {
    uint8_t inZone;
    uint64_t postingOffset;
    std::vector<uint16_t> postingSizes;
  };


  struct Entry
  {
    //Entry(uint32_t dOfs = 0, uint32_t pOfs = 0/*, uint8_t iz = 0*/) :
      //docIdOffset(dOfs), postingOffset(pOfs), inZone(iz) { }
    uint32_t docIdOffset;
    PostingInfo postingInfo;
  };


  struct InvIdxItem
  {
    uint32_t tokId;
    uint64_t offset;
    double idf;
    //std::vector<Entry> entries;
  };



  struct Posting
  {
    Posting() : begin(0), end(0) { }
    //Posting(void* src, uint64_t seek, uint16_t size);
    Posting(std::istream& is, uint64_t seek, uint16_t size);
    const uint16_t* begin;
    const uint16_t* end;
    private:
      std::shared_ptr<uint16_t> _data;
  };

  class PostingStorage 
  {
    public:
      PostingStorage(const Config& config) :
        _tZonesCnt(config.textZones.size())
        //_ifs(config.indexDataPath + config.postingsFile, std::ios::in | std::ios::binary)
      { _load(config.indexDataPath + config.postingsFile); }
      //Posting getPosting(uint32_t offset, uint8_t tZoneId);
      //uint16_t getPostingSize(uint32_t offset) const;
      void getFullPosting(const Entry& entry, std::vector<Posting>* res);
      //friend std::istream& (::operator>>)(std::istream& ifs, PostingStorage& ps);
    protected:
      PostingStorage() { }
      void _load(const std::string& path);

      std::ifstream _ifs;
      //void *_src;
      uint8_t _tZonesCnt;
      //uint32_t _commonSize;
      //std::vector<uint16_t> _postingSizes;
  };



  class DocStorage
  {
    public:
      DocStorage(const Config& config) :
        _nZones(config.numZones.size()), _tZonesWCnt(config.textZones.size())
      {
        _load(config.indexDataPath + config.docStoreFile);
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
      //DocStorage(uint8_t nZonesCnt, uint8_t tZonesCnt) :
      //  _nZones(nZonesCnt), _tZonesWCnt(tZonesCnt) { }
      DocStorage() { }
      uint32_t size() { return _docIds.size(); }
      void _load(const std::string& dataPath, bool isAppendMode = true);
      //
      std::vector<uint32_t> _docIds;
      std::vector< std::vector<uint32_t> > _nZones;
      std::vector< std::vector<uint16_t> > _tZonesWCnt;
  };


  struct RawDoc {
    RawDoc() { }
    RawDoc(std::map<std::string,std::string>& zones);
    std::string title;
    std::string videoId;
    std::string subtitles;
    std::string time;
    void getPhrases(std::vector< std::pair<std::string,double> >& phrases);
  };


  class DocDatabase {
    public:
      DocDatabase();
      ~DocDatabase();
      void open(const std::string& path);
      bool getDoc(uint32_t key, RawDoc* res);
      void putDoc(uint32_t key, RawDoc& doc);
    private:
      leveldb::DB *_db;
  };



  class InvertIndex {
    public:
      static const uint32_t unexistingToken;
      InvertIndex() : _pConfig(0), _pPostingStore(0), _pDocStore(0) { }
      ~InvertIndex();
      void configure(const std::string& pathToConfig);
      uint32_t findTknIdx(const std::string& word);
      uint32_t getDocId(const Entry& entry);
      uint32_t getNZone(const Entry& entry, uint8_t nZoneId);
      void getEntries(uint32_t tknIdx, std::vector<Entry>* res);
      double getIDF(uint32_t tknIdx);
      double getIDF(Index::Entry tknIdx);
      //double getTF(const Entry& entry, uint8_t tZoneId);
      void getZonesTf(const Entry& entry, std::vector<double>& tfVec);
      Posting getPosting(const Entry& entry, uint8_t tZoneId);
      void getFullPosting(const Entry& entry, std::vector<Posting>* res);
      void getRawDoc(uint32_t docId, RawDoc* res);

    private:
      //void _load();
      void _readInvIdxItem(std::ifstream& ifs, InvIdxItem* item);
      void _readInvIdxItemOpt(std::ifstream& ifs, std::ifstream& ifsForOpt, InvIdxItem* item);

      const Config* _pConfig;
      std::vector<InvIdxItem> _invIdx;
      std::ifstream _invIdxStream;
      //std::vector< std::vector<uint16_t> > _idf;
      PostingStorage* _pPostingStore;
      DocStorage* _pDocStore;
      DocDatabase _docDB;
  };


  void intersectEntries(std::vector< std::vector<Entry> >& input, std::vector< std::vector<Entry> >* output);
  //bool compareEntriesByTf(const Index::Entry& e1, const Index::Entry& e2);


  struct Exception : public std::exception
  {
    std::string errMsg;
    Exception(const std::string& msg) { errMsg = msg; }
    const char* what() const throw() { return errMsg.c_str(); }
  };


} // namespace Index

#endif // __INDEX_STRUCT_H__
