#ifndef __INDEX_STRUCT_H__
#define __INDEX_STRUCT_H__

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

//namespace Index { class PostingStorage; }
//std::istream& operator>>(std::istream& ifs, Index::PostingStorage& ps);


namespace Index {

  const uint8_t i2mask[8] = { 0, 2, 4, 8, 16, 32, 64, 128 };

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
    std::string documentDb;
    std::string rawDataPath;
    std::string docStoreFile;
    std::string postingsFile;
    std::string indexDataPath;
    std::string invertIndexFile;
    //
    std::vector<Zone*> numZones;
    std::vector<Zone*> textZones;
    std::vector<Zone*> trashZones;
  };



  struct Entry
  {
    Entry(uint32_t dOfs = 0, uint32_t pOfs = 0, uint32_t pSOfs = 0, uint8_t iz = 0) :
      docIdOffset(dOfs), postingOffset(pOfs), postingsSizeOffset(pSOfs), inZone(iz) { }
    uint32_t docIdOffset;
    uint32_t postingOffset;
    uint32_t postingsSizeOffset;
    uint8_t inZone;
  };


  struct InvIdxItem
  {
    void read(std::ifstream& ifs);
    //
    uint32_t tokId;
    std::vector<Entry> entries;
  };



  struct Posting
  {
    Posting() : begin(0), end(0) { }
    //Posting(uint16_t* b, uint16_t* e) : begin(b), end(e) { }
    Posting(std::istream& is, uint32_t seek, uint32_t size);
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
      uint16_t getPostingSize(uint32_t offset) const;
      std::vector<Posting> getFullPosting(const Entry& entry);
      //friend std::istream& (::operator>>)(std::istream& ifs, PostingStorage& ps);
    protected:
      PostingStorage() { }
      void _load(const std::string& path);

      //std::ofstream _ofs;
      std::ifstream _ifs;
      uint8_t _tZonesCnt;
      uint32_t _commonSize;
      std::vector<uint16_t> _postingSizes;
  };



  class DocStorage
  {
    public:
      DocStorage(const Config& config) {
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



  class InvertIndex {
    public:
      InvertIndex() : _pConfig(0), _pPostingStore(0), _pDocStore(0) { }
      ~InvertIndex();
      void configure(const std::string& pathToConfig);
      uint32_t findTknIdx(const std::string& word);
      uint32_t getDocId(const Entry& entry);
      uint32_t getNZone(const Entry& entry, uint8_t nZoneId); 
      std::vector<Entry> getEntries(uint32_t tknIdx);
      double getIDF(uint32_t tknIdx, uint8_t tZoneId);
      double getTF(const Entry& entry, uint8_t tZoneId);
      //Posting getPosting(const Entry& entry, uint8_t tZoneId);
      std::vector<Posting> getFullPosting(const Entry& entry);

    private:
      //void _load();

      const Config* _pConfig;
      std::vector<InvIdxItem> _invIdx;
      std::vector< std::vector<uint16_t> > _idf;
      PostingStorage* _pPostingStore;
      DocStorage* _pDocStore;
  };


  void intersectEntries(std::vector< std::vector<Entry> >& input, std::vector< std::vector<Entry> >& output);


  struct Exception : public std::exception
  {
    std::string errMsg;
    Exception(const std::string& msg) { errMsg = msg; }
    const char* what() const throw() { return errMsg.c_str(); }
  };


} // namespace Index

#endif // __INDEX_STRUCT_H__
