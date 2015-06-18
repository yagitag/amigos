#pragma once

#include<string>
#include<vector>
#include "../../../include/indexer/index_struct.h"

struct Document
{
    std::string videoId;
    std::string title;
    std::vector< std::pair< std::string, double > > subtitles;
};

class Searcher
{
    Index::InvertIndex index;
//    Ranker ranker;

public:
    Searcher() { };
    void configure( const std::string &path_to_config );
    
    void search( const std::vector< std::string > &tokens, std::vector<Document> &docs_output ) ;
    
    
};
