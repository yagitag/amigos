#ifndef __INDEXER_H__
#define __INDEXER_H__

#include <unordered_map>
#include <exception>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>

#include "MurmurHash2.h"
#include "tinyxml2_ext.hpp"
#include "index_struct.h"


class DynDocStorage : public Index::DocStorage
{
  public:
    DynDocStorage(uint8_t nZones, uint8_t tZones) : DocStorage(nZones, tZones) { }
    void addDoc(uint32_t docId, const std::vector<uint32_t>& nZones, const std::vector<uint16_t>& tZonesWCnt);
    void load(const std::string& dataPath);
    void save(const std::string& dataPath);
    size_t size();
};


class DynPostingStore : public Index::PostingStorage
{
  public:
    uint32_t addTokenPosting(const std::vector< std::vector<uint16_t> >& zonesPosting);
    size_t size();
    friend std::ofstream& operator<<(std::ofstream& ofs, const DynPostingStore& postingStore);
};




class Indexer
{
  public:
    Indexer(const Index::Config& config, uint64_t maxMemoryUsage = 1000000000);
    //virtual ~Indexer();
    void cook();

  private:
    void _parseRawData();
    void _parseZone(Index::Zone& zone, const tinyxml2::XMLElement* tiElem);
    void _extractTextZones(const tinyxml2::XMLElement* tiElem, std::vector<uint16_t>& wordsCnt);
    void _extractNumZones(const tinyxml2::XMLElement* tiElem, uint32_t docId, std::vector<uint16_t>& wordsCnt);
    bool _hasSpace();
    std::string _findFirstEmptyFile();
    void _flushToDisk(); //EXECPT _docStore
    void _addToStatDict(const std::string& word);

    const uint64_t _maxMemSize;
    uint64_t _charsCnt, _entriesCnt;
    //
    const Index::Config& _config;
    //
    std::map< uint32_t,std::vector<Index::Entry> > _invIdx;
    std::unordered_map<std::string,uint32_t> _statDict;
    DynDocStorage _docStore;
    DynPostingStore _postingStore;
};


#endif // __INDEXER_H__
