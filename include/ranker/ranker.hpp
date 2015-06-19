#include <vector>
#include <list>
#include "../../../include/indexer/index_struct.h"

class Ranker
{
public:

    Ranker() {};
    void get_list_of_sorted_docid_by_rank(Index::InvertIndex &index, std::vector< std::vector<Index::Entry> > &entries_by_doc, std::list< size_t > &docid_by_rank);
    
};
