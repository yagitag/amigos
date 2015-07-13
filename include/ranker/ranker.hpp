#include <vector>
#include <list>
#include "../../../include/indexer/index_struct.h"

class Ranker
{
public:

    enum TextZone
    {
      TITLE = 0,
      DESC = 1,
      SUB = 2
    };

    enum NumZone
    {
      VIEWS_CNT,
      LIKES,
      DISLIKES,
      EN_TYPE,
      RU_TYPE,
      VTYPE,
      LOUDNESS,
      IS_FAMILY_FRIENDLY,
      QUALITY,
      SUBSCRIBERS_CNT
    };

    enum VideoQuality
    {
      Q720,
      Q360
    };

    enum VideoType {
      SCIENCE,
      SHOW,
      HOWTO,
      BLOG,
      ENTERTAINMENT,
      EDUCATION,
      NEWS,
      GAMING,
      NONPROFIT,
      COMEDY,
      VEHICLES,
      ANIMAL,
      MUSIC,
      FILM,
      SPORT,
      EVENT,
      MOVIE,
      TRAILER
    };

    enum SubType {
      AUTO,
      TRAN,
      ORIG
    };

    Ranker() {};
    void get_list_of_sorted_docid_by_rank(Index::InvertIndex &index, std::vector< std::vector<Index::Entry> > &entries_by_doc, std::vector< std::pair < uint32_t, std::pair< float, float > > > &docid_by_rank);
    double get_draft_rank(Index::InvertIndex &index, const std::vector<Index::Entry> &entries_by_doc, const std::vector<double> &idfs);
    void draft_ranking(Index::InvertIndex &index, const std::vector< std::vector<Index::Entry> > &entries_by_docs, const std::vector<double> &idfs, std::vector< std::vector<Index::Entry> > &entries_by_doc_res);
    
};


