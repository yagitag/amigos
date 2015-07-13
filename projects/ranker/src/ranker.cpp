#include "../../../include/ranker/ranker.hpp"
#include "../../../include/indexer/index_struct.h"
#include <set> 
#include <utility> 
#include <algorithm>

using namespace std;
using namespace Index;

#define MAX_WINDOW_SIZE 10
#define MAX_DRAFT_DOCS 200 //MAGIK!!!

void get_positions_for_zone( InvertIndex &index, const vector<Entry> &entries, size_t zoneId, list< pair< uint16_t , size_t > > &positions, std::vector< std::vector<Posting> > &postings_by_entries )
{
    
    for( size_t i = 0; i < entries.size(); ++i )
    {
        const Entry &entry = entries[i];
        //std::vector<Posting> postings_by_zone;
        //index.getFullPosting(entry, &postings_by_zone);
        Posting &postings = postings_by_entries[i][zoneId]; //check it!!!
        
        list< pair< uint16_t, size_t > >::iterator it_list = positions.begin();
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
    std::vector<std::vector<Posting> > postings_by_entries(entries.size());
    for(size_t i = 0; i < entries.size(); ++i)
    {
        index.getFullPosting(entries[i], &postings_by_entries[i]);
    }
    // для каждой зоны собираем упорядоченный список пар позиция-энтрис
    list< pair< uint16_t , size_t > > all_pos_by_entries_title;
    get_positions_for_zone(index, entries, 0, all_pos_by_entries_title, postings_by_entries);
    float rank_title = get_rank(all_pos_by_entries_title, entries.size());
//    list< pair< uint16_t , size_t > > all_pos_by_entries_disc;
//    get_positions_for_zone( index, entries, 1, all_pos_by_entries_disc );
    list< pair< uint16_t , size_t > > all_pos_by_entries_subs;
    get_positions_for_zone(index, entries, 2, all_pos_by_entries_subs, postings_by_entries);
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
        const vector<Entry> &entries = entries_by_doc[i];
        if( entries.empty() ) continue; // can not be ?
        pair< float, float > doc_rank = get_doc_rank( index, entries );
        auto docId = index.getDocId(entries[0]);
        docid_by_rank.push_back(make_pair( docId, doc_rank ));

    }
    sort(docid_by_rank.begin(), docid_by_rank.end(), comp_title);
    auto it1 = lower_bound(docid_by_rank.begin(), docid_by_rank.end(), 5000, title_only_more);
    auto it2 = lower_bound(docid_by_rank.begin(), docid_by_rank.end(), 500, title_only_more);
    sort(docid_by_rank.begin(), it1, comp_subs);
    sort(it1, it2, comp_subs);
    sort(it2, docid_by_rank.end(), comp_subs);
    std::cout << "FINISHIG RANKING" << std::endl;
}


double Ranker::get_draft_rank(InvertIndex &index, const vector<Entry> &entries_by_doc, const vector<double> &idfs, const vector<uint32_t> max_nzones_vals)
{
    double rank = 0.;
    std::vector<double> zonesTf;
    for (size_t i = 0; i < entries_by_doc.size(); ++i)
    {
      index.getZonesTf(entries_by_doc[i], zonesTf);
      rank += idfs[i] * (zonesTf[TITLE] + zonesTf[DESC] * 0.2 + zonesTf[SUB] * 5);
    }
    if (!entries_by_doc.empty())
    {
      auto& entry = entries_by_doc[0];
      rank += index.getNZone(entry, Ranker::VIEWS_CNT) / max_nzones_vals[Ranker::VIEWS_CNT];
    }
    return rank;
}

bool more_rank(const pair< vector<Entry>, double /*draft_rank*/> &e1 ,const pair< vector<Entry>, double /*draft_rank*/> &e2)
{
    return e1.second > e2.second;
}


uint32_t getMaxNZone(InvertIndex& index, const vector< vector<Entry> >& entries_by_doc, Ranker::NumZone nzone)
{
  uint32_t max = 0;
  for (size_t i = 0; i < entries_by_doc.size(); ++i)
  {
    max = std::max(index.getNZone(entries_by_doc[i].front(), nzone), max); // все entry сгруппированы по документам, поэтому берем первый подходящий
  }
  return max;
}


void Ranker::draft_ranking(InvertIndex &index, const vector< vector<Entry> > &entries_by_docs, const vector<double> &idfs, vector< vector<Entry> > &entries_by_doc_res)
{
    vector<uint32_t> max_nzones_vals(numZonesCnt);
    max_nzones_vals[VIEWS_CNT] = getMaxNZone(index, entries_by_docs, VIEWS_CNT);

    vector< pair< vector<Entry>, double /*draft_rank*/> > enries_by_docs_with_rank(entries_by_docs.size());
    for(size_t i = 0; i < entries_by_docs.size(); ++i)
    {
        const vector<Entry> &entries_by_doc = entries_by_docs[i];
        enries_by_docs_with_rank[i].first = entries_by_doc;
        enries_by_docs_with_rank[i].second = get_draft_rank(index, entries_by_doc, idfs, max_nzones_vals);
    }
    sort(enries_by_docs_with_rank.begin(), enries_by_docs_with_rank.end(), more_rank);
    size_t num = entries_by_docs.size() > MAX_DRAFT_DOCS ? MAX_DRAFT_DOCS : entries_by_docs.size();
    entries_by_doc_res.resize(num);
    for(size_t i = 0; i < num; ++i)
        entries_by_doc_res[i] = enries_by_docs_with_rank[i].first;
}
