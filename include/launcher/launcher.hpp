#pragma once

#include <string>
#include <vector>
#include "../../../include/searcher/searcher.hpp"
#include "../../../include/common/bigramer.hpp"

class Launcher
{
public:
    Launcher() {};
    void configure( const std::string &path_to_config, const std::string& path_to_stopwords );

    void launch_searcher( std::string &query, std::vector<uint32_t> &docsId ); //DO IT!
    void get_snippets( std::string &query, uint32_t docId, std::vector< Snippet > &snippets, uint32_t snippets_num );
    void get_doc( uint32_t docId, Document &doc );

private:
    Common::BigRamer bi_gramer;
    Searcher searcher;
};
