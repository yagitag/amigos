#ifndef __INDEXER_H__
#define __INDEXER_H__

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "../../../contrib/tinyxml2/tinyxml2.h"


const tinyxml2::XMLElement* getNecessaryTag(const tinyxml2::XMLElement& tiElem, const std::string& tag);
tinyxml2::XMLDocument openXmlFile(const std::string& filePath);

class Zone
{
  public:
    Zone(const tinyxml2::XMLElement& tiElem);
    friend std::ostream& operator<<(std::ostream& os, const Zone& zone);

    std::string name;
    std::vector<std::string> path;
    bool isOptional;
};
//
//class TextZone : public Zone
//{
//  bool isForIndexer;
//};
//
//class NumZone : public Zone
//{
//  unsigned char bitLen;
//};



class Indexer
{
  public:
    Indexer(const std::string& configPath);
    virtual ~Indexer();
    void tmpPrint();
    void parseRawData();
    void extractTextZones(const tinyxml2::XMLElement* tiElem);
    void extractNumZones(const tinyxml2::XMLElement* tiElem);
    //..

    struct Exception : public std::exception
    {
      std::string errMsg;
      Exception(const std::string& msg) { errMsg = msg; }
      const char* what() const throw() { return errMsg.c_str(); }
    };

  private:

    std::string _docId;
    std::vector<Zone*> _textZones;
    std::vector<Zone*> _numZones;
    std::vector<Zone*> _trashZones;
    std::string _rawDataPath;
    std::string _indexDataPath;
    std::string _plainIndexFile;
    std::string _invertIndexFile;
};

#endif // __INDEXER_H__
