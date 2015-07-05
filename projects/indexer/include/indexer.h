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
#include "../../../include/indexer/index_struct.h"
#include "../../../include/common/bigramer.hpp"


class DynDocStorage : public Index::DocStorage
{
  public:
    //DynDocStorage(uint8_t nZones, uint8_t tZones) : DocStorage(nZones, tZones) { }
    DynDocStorage(const Index::Config& config);
    ~DynDocStorage();
    void addDoc(uint32_t docId, const std::vector<uint32_t>& nZones, const std::vector<uint16_t>& tZonesWCnt);
    //void load(const std::string& dataPath);
    //void save(const std::string& dataPath);
    size_t size();
  private:

    std::string _path;
};


//class DynPostingStore;
//std::ostream& operator<<(std::ostream& ofs, DynPostingStore& postingStore);

//class RawIndex;
class DynPostingStore : public Index::PostingStorage
{
  public:
    DynPostingStore(const Index::Config& config);
    ~DynPostingStore();
    void addTokenPosting(std::vector< std::vector<uint16_t> >& zonesPosting, uint64_t* postingOffset);
    //size_t size();
    //void mergeWith(const std::string& path);
    //void copyPosting(std::ofstream& ofs);
    //friend std::ostream& operator<<(std::ostream& ofs, DynPostingStore& postingStore);
    //friend class RawIndex;
  protected:
    std::ofstream _ofs;
    std::string _path;
    uint32_t _commonSize;
};



//class EntriesComparator
//{
//  public:
//    EntriesComparator(const DocStorage& dds, const PostingStorage& dps);
//    bool operator()(const Entry& e1, const Entry& e2);
//  private:
//    const DocStorage& _ds;
//    const PostingStorage& _ps;
//};



class Indexer
{
  public:
    Indexer(const Index::Config& config, uint64_t maxMemoryUsage = 1000000000);
    ~Indexer();
    void cook();

  private:
    void _parseRawData();
    void _parseZone(Index::Zone& zone, const tinyxml2::XMLElement* tiElem);
    void _extractTextZones(const tinyxml2::XMLElement* tiElem, std::vector<uint16_t>& wordsCnt, std::map<std::string,std::string>& forSave);
    void _extractNumZones(const tinyxml2::XMLElement* tiElem, std::vector<uint32_t>& numZones, std::map<std::string,std::string>& forSave);
    bool _hasSpace();
    std::string _findFirstEmptyFile();
    void _flushToDisk(); //EXECPT _docStore
    void _mergeIndexes();
    void _addToStatDict(const std::string& word);
    //friend class EntriesComparator;

    const uint64_t _maxMemSize;
    uint64_t _charsCnt, _entriesCnt;
    //
    const Index::Config& _config;
    //
    std::map< uint32_t,std::vector<Index::Entry> > _invIdx;
    std::unordered_map<std::string,uint32_t> _statDict;
    DynDocStorage _docStore;
    DynPostingStore _postingStore;
    Index::DocDatabase _docDB;
    Common::BigRamer _bigramer;
};


#endif // __INDEXER_H__
