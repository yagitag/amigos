#include "../../../include/launcher/launcher.hpp"
#include "../../../include/common/normalizer.hpp"
#include "../../../include/searcher/searcher.hpp"

using namespace std;
using namespace Common;

void Launcher::configure( const string& path_to_config, const string& path_to_stopwords )
{                                                       
    searcher.configure( path_to_config );                  
    if ( path_to_stopwords != "" )
        bi_gramer.configure ( path_to_stopwords );
}                                                       

void Launcher::launch_searcher( std::string &query, std::vector<Document> &docs )
{
    vector< string > norm_words;
    terminate2vec( query, norm_words );

    vector< string > tokens;
    bi_gramer.bigram2vec( norm_words, tokens );

    searcher.search( tokens, docs );
}

