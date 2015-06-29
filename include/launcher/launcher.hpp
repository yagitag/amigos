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

    void launch_searcher( std::string &query, std::vector<Document> &docs ); //DO IT!

private:
    Common::BigRamer bi_gramer;
    Searcher searcher;
};
