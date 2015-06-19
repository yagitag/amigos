#include "../../../include/ranker/ranker.hpp"
#include "../../../include/indexer/index_struct.h"
#include <utility> 
#include <set> 

using namespace std;
using namespace Index;

#define MAX_WINDOW_SIZE 10

void get_positions_for_zone( InvertIndex &index, const vector<Entry> &entries, size_t zoneId, list< pair< uint16_t , size_t > > &positions )
{
    
    for( size_t i = 0; i < entries.size(); ++i )
    {
        const Entry &entry = entries[i];
        std::vector<Posting> postings_by_zone = index.getFullPosting(entry);
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
/*
size_t get_rank(list< pair< uint16_t , size_t > > &all_pos, size_t window_size)
{
    if (window_size > MAX_WINDOW_SIZE) window_size = MAX_WINDOW_SIZE;
    if (all_pos.size() == 0) return 0;
    
    get_rank(list< pair< uint16_t , size_t > >::iteraor it = all_pos.begin();
    uint16_t start, end;
    start = end = it->first;
    int16_t old_dist = end-start;
    int16_t new_dist = end-start;
    set< size_t > wordsId_in_passage;
    for( ; it != all_pos.end(); ++it )
    {
        new_dist = it->first - start;
        if( new_dist >= window_size )
        {
            
        }
    }
}
*/
size_t get_doc_rank( InvertIndex &index, const vector<Entry> &entries )
{
    // для каждой зоны собираем упорядоченный список пар позиция-энтрис
    list< pair< uint16_t , size_t > > all_pos_by_entries_title;
    get_positions_for_zone( index, entries, 0, all_pos_by_entries_title );
    //size_t rank_title = get_rank(all_pos_by_entries_title, entries.size());

    list< pair< uint16_t , size_t > > all_pos_by_entries_disc;
    get_positions_for_zone( index, entries, 1, all_pos_by_entries_disc );
    list< pair< uint16_t , size_t > > all_pos_by_entries_subs;
    get_positions_for_zone( index, entries, 2, all_pos_by_entries_subs );
}

void Ranker::get_list_of_sorted_docid_by_rank( InvertIndex &index, vector< vector<Entry> > &entries_by_doc, list< size_t > &docid_by_rank)
{
    // собираем упорядоченный список всех позиций всех входжений 
    list< pair< uint16_t , size_t > > all_postings_by_entries;
    for( size_t i = 0; i < entries_by_doc.size(); ++i )
    {
        const vector<Entry> &entries = entries_by_doc[i];
        size_t doc_rank = get_doc_rank( index, entries );
        /*
        for ( Entry &entry : entries )
        {
            vector<Posting> postings = index.getFullPosting(entry);
            for ( Posting &posting : postings )
            {
                insert( posting, i, all_postings_by_entries );
            }
        }
*/
    }
}
