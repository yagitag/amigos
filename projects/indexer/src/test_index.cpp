#include "../../../include/indexer/index_struct.h"

#include <iostream>
#include <iomanip>
#include <string>
#include "loc.hpp"

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
    std::string word;
    while (std::cin >> word) {
      auto tokId = index.findTknIdx(word);
      std::vector<Index::Entry> entries = index.getEntries(tokId);
      uint32_t docId;
      for (auto& entry: entries) {
        docId = index.getDocId(entry);
        std::cout << docId << std::endl;
        auto doc = index.getRawDoc(docId);
        std::cout << "VIDEO_ID:\t" << doc->videoId << std::endl;
        std::cout << "TITLE:\t" << doc->title << std::endl;
        delete doc;
        std::cout << "--------------------------------------------------------" << std::endl;
        std::vector<Index::Posting> postings;
        postings = index.getFullPosting(entry);
        for (size_t i = 0; i < postings.size(); ++i) {
          std::cout << std::left << std::setw(15) << config.textZones[i]->name;
          for (auto it = postings[i].begin; it != postings[i].end; ++it) {
            std::cout << ' ' << *it;
          }
          std::cout << std::endl;
        }
        std::cout << "--------------------------------------------------------" << std::endl;
      }
    }
  }
  catch (std::exception& e) {
    std::cerr << "Catch the exception: " << e.what() << std::endl;
  }
  return 0;
}
