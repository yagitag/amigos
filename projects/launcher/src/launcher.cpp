#include "../../../include/launcher/launcher.hpp"
#include "../../../include/common/normalizer.hpp"
#include "../../../include/searcher/searcher.hpp"

using namespace std;
using namespace Common;

const uint32_t MAX_TOKEN_CNT = 10;

void Launcher::configure( const string& path_to_config, const string& path_to_stopwords )
{
    searcher.configure( path_to_config );
    init_locale("en_US.UTF-8");
    if ( path_to_stopwords != "" )
        bi_gramer.configure ( path_to_stopwords );
}

void Launcher::launch_searcher( std::string &query, std::vector<uint32_t> &docsId )
{
    vector< string > norm_words;
    terminate2vec( query, norm_words );
    if (norm_words.size() > MAX_TOKEN_CNT) norm_words.resize(MAX_TOKEN_CNT);

    vector< string > tokens;
    bi_gramer.bigram2vec( norm_words, tokens );

    for ( string token: tokens )
    {
        std::cout << "TOKEN: " << token << std::endl;
    }
    searcher.search( tokens, docsId );
}

void Launcher::get_snippets( std::string &query, uint32_t docId, std::vector< Snippet > &snippets, uint32_t snippets_num )
{
    vector< string > norm_words;
    terminate2vec( query, norm_words, false );
    
    searcher.get_snippets( docId, norm_words, snippets, snippets_num);
}

void Launcher::get_doc( uint32_t docId, Document &doc )
{
    searcher.get_doc(docId, doc);
    /*
    Index::RawDoc raw_doc;
    index.getRawDoc( docId, &raw_doc );
    doc.videoId = raw_doc.videoId;
    doc.title = raw_doc.title;
    doc.docId = docid_with_rank.first;
    */
}
