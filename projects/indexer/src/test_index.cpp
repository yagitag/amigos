#include "../../../include/indexer/index_struct.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string>
#include "loc.hpp"

bool compareEntriesByTf(const Index::Entry& e1, const Index::Entry& e2) {
  for (size_t i = 0; i < e1.zoneTf.size(); ++i) {
    if (e1.zoneTf[i] > e2.zoneTf[i]) return true;
    else if (e1.zoneTf[i] < e2.zoneTf[i]) return false;
  }
  return false;
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
    Index::InvertIndex index;
    index.configure(configPath);
    Index::Config config(configPath);
    //
    std::cout << "INIT FINISHED" << std::endl;
    //
    std::string word;
    Index::RawDoc doc;
    std::vector<Index::Entry> entries;
    std::vector<Index::Posting> postings;
    while (std::cin >> word) {
      auto tokId = index.findTknIdx(word);
      if (tokId != Index::InvertIndex::unexistingToken) {
        index.getEntries(tokId, &entries);
        std::cout << "ENTRIES COUNT WITHOUT CUT: " << entries.size() << std::endl;
        std::sort(entries.begin(), entries.end(), compareEntriesByTf);
        //entries.resize(2);
        uint32_t docId;
        for (auto& entry: entries) {
          docId = index.getDocId(entry);
          std::cout << "--------------------------------------------------------" << std::endl;
          std::cout << docId << std::endl;
          index.getRawDoc(docId, &doc);
          std::cout << "VIDEO_ID:\t" << doc.videoId << std::endl;
          std::cout << "TITLE:\t" << doc.title << std::endl;
          std::vector< std::pair<std::string,double> > phrases;
          //doc->getPhrases(phrases);
          for (const auto& phrase: phrases) {
            std::cout << phrase.second << ": " << phrase.first << std::endl;
          }
          postings.clear();
          index.getFullPosting(entry, &postings);
          for (size_t i = 0; i < postings.size(); ++i) {
            std::cout << std::left << std::setw(15) << config.textZones[i]->name;
            for (auto it = postings[i].begin; it != postings[i].end; ++it) {
              std::cout << ' ' << *it;
            }
            std::cout << std::endl;
          }
        }
        entries.clear();
      }
      else {
        std::cout << "NOTHING" << std::endl;
      }
      std::cout << "--------------------------------------------------------" << std::endl;
    }
  }
  catch (std::exception& e) {
    std::cerr << "Catch the exception: " << e.what() << std::endl;
  }
  return 0;
}
