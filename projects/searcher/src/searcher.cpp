#include "../../../include/searcher/searcher.hpp"
#include "../../../include/indexer/index_struct.h"
#include <list>

using namespace std;
using namespace Index;

void Searcher::configure( const string& path_to_config ) 
{
    index.configure( path_to_config );
}

void Searcher::search( const vector< string > &tokens, std::vector<Document> &docs )
{
    // это вектор векторов Энтрисов!
    vector< vector<Entry> > enties_by_token;
    for (size_t i = 0; i < tokens.size(); ++i )
    {
        uint32_t tknId = index.findTknIdx( tokens[i] );
        enties_by_token.push_back(index.getEntries(tknId));
    }

    // а это еще один вектор векторов Энтрисов
    vector< vector<Entry> > entries_by_doc;
    intersectEntries( enties_by_token, entries_by_doc);

    for ( size_t i = 0; i < entries_by_doc.size(); ++i )
    {
        if( entries_by_doc[i].size() )
        {
            RawDoc *raw_doc = index.getRawDoc( index.getDocId(entries_by_doc[i][0]) );
            Document doc;
            doc.videoId = raw_doc->videoId;
            doc.title = raw_doc->title;
            std::vector< std::pair<std::string, double> > phrases;
            raw_doc->getPhrases( phrases );
            for( size_t i = 0; i < 3 && i < phrases.size(); ++i )
            {
                doc.subtitles.push_back(phrases[i]);
            }
            docs.push_back(doc);
        }
    }
/*
    list docid_by_rank;
    ranker.get_list_of_sorted_docid_by_rank(entries_by_doc, docid_by_rank); // YO

*/
    
    
}
