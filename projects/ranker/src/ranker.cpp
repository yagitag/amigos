#include "../../../include/ranker/ranker.hpp"
#include "../../../include/indexer/index_struct.h"
#include <utility> 
#include <set> 
#include <algorithm>

using namespace std;
using namespace Index;

#define MAX_WINDOW_SIZE 10

void get_positions_for_zone( InvertIndex &index, const vector<Entry> &entries, size_t zoneId, list< pair< uint16_t , size_t > > &positions )
{
    
    for( size_t i = 0; i < entries.size(); ++i )
    {
        const Entry &entry = entries[i];
        std::vector<Posting> postings_by_zone;
        index.getFullPosting(entry, &postings_by_zone);
        Posting postings = postings_by_zone[zoneId]; //check it!!!
        
        list< pair< uint16_t , size_t > >::iterator it_list = positions.begin();
        const uint16_t *entry_pos = postings.begin;
        while( it_list != positions.end() && entry_pos != postings.end )
        {
            if( *entry_pos > it_list->first )
                it_list++;
            else 
            {
                positions.insert( it_list, make_pair(*entry_pos, i) );
                entry_pos++;
            }
        }
            if( entry_pos != postings.end )
        {
            for( ; entry_pos != postings.end; ++entry_pos )
            {
                positions.insert( positions.end(), make_pair(*entry_pos, i) );
            }
        }
    }
}

float calculate_bonus(list< pair< uint16_t , size_t > >::iterator &start_w, list< pair< uint16_t , size_t > >::iterator &end_w, size_t window_size)
{
    set< size_t > wordsId_in_passage;
    for( list< pair< uint16_t , size_t > >::iterator it = start_w; it != end_w; ++it )
        wordsId_in_passage.insert(it->second);   
    return wordsId_in_passage.size() >= window_size ? 1000 : 200.0*float(wordsId_in_passage.size())/float(window_size); // MAGIC!!!!
}

float get_rank(list< pair< uint16_t , size_t > > &all_pos, size_t window_size)
{
    if (window_size > MAX_WINDOW_SIZE) window_size = MAX_WINDOW_SIZE;
    if (all_pos.size() == 0) return 0;
    
    //list< pair< uint16_t , size_t > >::iterator it = all_pos.begin();
    list< pair< uint16_t , size_t > >::iterator start_w, end_w;
    start_w = end_w = all_pos.begin();
  //  int16_t old_dist = end-start;
  //  int16_t new_dist = end-start;
    //set< size_t > wordsId_in_passage;
    float result = 0;
    while(end_w != all_pos.end()) 
    {
        if(end_w->first - start_w->first < window_size)
        {
            if(++end_w != all_pos.end())
            {
                if(end_w->first - start_w->first < window_size)
                {
                    continue;
                }
                else
                {
                    result += calculate_bonus(start_w, end_w, window_size);
                    ++start_w;
                }
            }
            else
            {
                result += calculate_bonus(start_w, end_w, window_size); // O M G !!!?
            }
        }
        else
        {
            ++start_w;
        }
    }
    return result;
}

pair< float, float > get_doc_rank( InvertIndex &index, const vector<Entry> &entries )
{
    // для каждой зоны собираем упорядоченный список пар позиция-энтрис
    list< pair< uint16_t , size_t > > all_pos_by_entries_title;
    get_positions_for_zone( index, entries, 0, all_pos_by_entries_title );
    float rank_title = get_rank(all_pos_by_entries_title, entries.size());
//    list< pair< uint16_t , size_t > > all_pos_by_entries_disc;
//    get_positions_for_zone( index, entries, 1, all_pos_by_entries_disc );
    list< pair< uint16_t , size_t > > all_pos_by_entries_subs;
    get_positions_for_zone( index, entries, 2, all_pos_by_entries_subs );
    float rank_subs = get_rank(all_pos_by_entries_subs, entries.size());

    return std::make_pair( rank_title*5, rank_subs ); // MORE MAGIC !!!!!
}

bool title_only_more( pair <size_t, pair< float, float > > i, float val )
{
    return val <= i.second.first;
}

bool comp_title( pair <size_t, pair< float, float > > x, pair <size_t, pair< float, float > > y)
{
    return x.second.first > y.second.first;
}

bool comp_subs( pair <size_t, pair< float, float > > x, pair <size_t, pair< float, float > > y)
{
    return x.second.second > y.second.second;
}

void Ranker::get_list_of_sorted_docid_by_rank( InvertIndex &index, vector< vector<Entry> > &entries_by_doc, vector< pair <uint32_t, pair< float, float > > > &docid_by_rank)
{
  std::cout << "START RANKING" << std::endl;
    for( size_t i = 0; i < entries_by_doc.size(); ++i )
    {
        std::cerr << i << std::endl;
        const vector<Entry> &entries = entries_by_doc[i];
        if( entries.empty() ) continue; // can not be ?
        pair< float, float > doc_rank = get_doc_rank( index, entries );
        auto docId = index.getDocId(entries[0]);
        docid_by_rank.push_back(make_pair( docId, doc_rank ));

    }
    sort(docid_by_rank.begin(), docid_by_rank.end(), comp_title);
    std::cout << "SORT BY TITLE" << std::endl;
    auto it1 = lower_bound(docid_by_rank.begin(), docid_by_rank.end(), 5000, title_only_more);
    auto it2 = lower_bound(docid_by_rank.begin(), docid_by_rank.end(), 500, title_only_more);
    sort(docid_by_rank.begin(), it1, comp_subs);
    sort(it1, it2, comp_subs);
    sort(it2, docid_by_rank.end(), comp_subs);
    std::cout << "FINISHIG RANKING" << std::endl;
}
