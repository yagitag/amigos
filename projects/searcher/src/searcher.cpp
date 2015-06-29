#include "../../../include/searcher/searcher.hpp"
#include "../../../include/indexer/index_struct.h"
#include <vector>

using namespace std;
using namespace Index;

void Searcher::configure( const string& path_to_config ) 
{
    index.configure( path_to_config );
}

void Searcher::search( const vector< string > &tokens, std::vector<Document> &docs )
{
    // это вектор векторов Энтрисов!
    vector< vector<Entry> > entries_by_token(tokens.size());
    for (size_t i = 0; i < tokens.size(); ++i )
    {
        uint32_t tknId = index.findTknIdx( tokens[i] );
        if ( tknId == Index::InvertIndex::unexistingToken ) return;
        index.getEntries(tknId, &(entries_by_token[i]));
    }

    // а это еще один вектор векторов Энтрисов
    vector< vector<Entry> > entries_by_doc;
    intersectEntries( entries_by_token, &entries_by_doc);
    std::cout << "entries_by_token: "  << entries_by_token.size() << std::endl;
    std::cout << "entries_by_doc: "  << entries_by_doc.size() << std::endl;

    vector< pair< uint32_t, pair< float, float > > > docid_by_rank;
    ranker.get_list_of_sorted_docid_by_rank(index, entries_by_doc, docid_by_rank); // YO

    for ( auto &docid_with_rank : docid_by_rank )
    {
        RawDoc raw_doc;
        index.getRawDoc( docid_with_rank.first, &raw_doc );
        Document doc;
        doc.videoId = raw_doc.videoId;
        doc.title = raw_doc.title;
        std::vector< std::pair<std::string, double> > phrases;
//        raw_doc->getPhrases( phrases );
        for( size_t i = 0; i < 3 && i < phrases.size(); ++i )
        {
            doc.subtitles.push_back(phrases[i]);
        }
        docs.push_back(doc);
    }

    for( auto &docid_with_rank : docid_by_rank )
    {
        RawDoc doc;
        index.getRawDoc(docid_with_rank.first, &doc);
        std::cerr << "RANK1: " << std::endl;
        std::cerr << docid_with_rank.second.first << std::endl;;
        std::cerr << "RANK2: " << std::endl;
        std::cerr << docid_with_rank.second.second << std::endl;;
        std::cerr << "TITLE: " << std::endl;
        std::cerr << doc.title << std::endl;
 //       std::cerr << "SUBS: " << std::endl;      
 //       std::cerr << doc.subtitles << std::endl;
        std::cerr << "ID: " << std::endl;        
        std::cerr << doc.videoId << std::endl;  
        std::cerr << "____________" << std::endl;        
    }
}
