#include "../../../include/searcher/searcher.hpp"
#include "../../../include/indexer/index_struct.h"
#include <vector>
#include <algorithm>

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
        doc.docId = docid_with_rank.first;
/*
        std::vector< std::pair<std::string, double> > phrases;
        raw_doc.getPhrases( phrases );
        for( size_t i = 0; i < 3 && i < phrases.size(); ++i )
        {
            doc.subtitles.push_back(phrases[i]);
        }
*/
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

typedef std::pair< uint32_t /*select_start_pos*/, uint32_t /*select_length*/ > Selection_t;
    
std::string get_query_part(const std::vector< std::string > &tokens, size_t start, size_t end)
{
    std::string query_part = tokens[start];
    for( size_t i = start+1; i < end; ++i)
    {
        query_part += " " + tokens[i];
    }
    return query_part;
} 

typedef std::pair<std::string, double> Phrase_t;
typedef std::pair< std::vector< Selection_t >, std::vector< size_t > /* selections_word_count_statistic */ > Selections_with_statistic_t;

void get_selections(const std::vector< std::string > &tokens, std::pair< Selections_with_statistic_t, Phrase_t > *selections_with_statistic)
{
    std::list< std::pair<Selection_t, size_t /* selection_size */ > >selects_with_sizes;
    for(size_t window_size = 1; window_size <= tokens.size(); ++window_size)
    {
        for(size_t start = 0, end = window_size; end <= tokens.size(); ++start, ++end)
        {
            std::string query_part   = get_query_part(tokens, start, end);
            std::string query_part_m = " " + query_part + " ";
            std::string subtitle_m   = " " + selections_with_statistic->second.first + " ";
            if(window_size == 1)
            {
                size_t found_pos = -1;
                while((found_pos = subtitle_m.find(query_part_m, found_pos+1)) != std::string::npos)
                    selects_with_sizes.push_back(std::make_pair(Selection_t(found_pos, query_part_m.size()-1), window_size));
            }
            else
            {
                for( std::list< std::pair<Selection_t, size_t> >::iterator it = selects_with_sizes.begin(); it != selects_with_sizes.end(); ++it )
                {
                    size_t start = it->first.first;
//                    size_t end  = start + it->first.second;
                    size_t new_end  = start + query_part_m.size();

                    if(new_end >= subtitle_m.size()) break; //CHECK IT !!!

                    if(std::string(&subtitle_m[start], &subtitle_m[new_end]) == query_part_m)
                    {
                        it->first.second = query_part_m.size()-1;
                        it->second = window_size;
                        // Проверяем, не захавали ли соседнее вхождение
                        if(++it != selects_with_sizes.end())
                        {
                            if(it->first.first < new_end) // CHECK IT !!!
                                selects_with_sizes.erase(it);
                        }
                        break;
                    }
                }
            }
        }
    }
    selections_with_statistic->first.second.resize(tokens.size());
    for( auto &select_and_sizes : selects_with_sizes )
    {
        selections_with_statistic->first.first.push_back(select_and_sizes.first);
        ++(selections_with_statistic->first.second[select_and_sizes.second]);
    }
}


bool snippet_ranker(std::pair< Selections_with_statistic_t, Phrase_t > s1, std::pair< Selections_with_statistic_t, Phrase_t > s2)
{
    for(size_t i = s1.first.second.size()-1; i >= 0; --i)
    {
        if(s1.first.second[i] > s2.first.second[i]) return true;
        if(s1.first.second[i] < s2.first.second[i]) return false;
    }
    return false;
}

void Searcher::get_snippets( uint32_t docId, const std::vector< std::string > &tokens, std::vector< Snippet > snippets, uint32_t snippets_num )
{
    RawDoc raw_doc;
    index.getRawDoc(docId, &raw_doc);
    std::vector<Phrase_t> phrases;
    raw_doc.getPhrases( phrases );

    std::vector< std::pair< Selections_with_statistic_t, Phrase_t > > all_selections_with_stat(phrases.size());
    for(size_t i = 0; i < phrases.size(); ++i)
    {
        all_selections_with_stat[i].second = phrases[i];
        get_selections(tokens, &all_selections_with_stat[i]);
    }
    std::sort(all_selections_with_stat.begin(), all_selections_with_stat.end(), snippet_ranker);
    for(size_t i = 0; i < snippets_num && i < all_selections_with_stat.size(); ++i)
    {
        Snippet snippet;
        snippet.subtitle = all_selections_with_stat[i].second;
        snippet.selections.assign(all_selections_with_stat[i].first.first.begin(), all_selections_with_stat[i].first.first.end());
    }
}
