#pragma once

#include<string>
#include<vector>
#include "../../../include/indexer/index_struct.h"
#include "../../../include/ranker/ranker.hpp"

struct Document
{
    std::string videoId;
    std::string title;
    uint32_t docId; // NOT FOR ANDRUSHA! FOR SNIPPETS!
    //std::vector< std::pair< std::string, double > > subtitles;
};

struct Snippet
{
    std::pair< std::string, double > subtitle;
    std::vector< std::pair< uint32_t, uint32_t > > selections; //vector < pair < selection_start, selection_length > > 
};

class Searcher
{
    Index::InvertIndex index;
    Ranker ranker;

public:
    Searcher() { };
    void configure( const std::string &path_to_config );
    
    void search( const std::vector< std::string > &tokens, std::vector<Document> &docs_output );
    void get_snippets( uint32_t docId, const std::vector< std::string > &tokens, std::vector< Snippet > snippets, uint32_t snippets_num );
};
