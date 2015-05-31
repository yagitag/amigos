#ifndef __BIGRAMER__HPP__
#define __BIGRAMER__HPP__

#include <string>
#include <fstream>
#include "normalizer.hpp"
#include "tokenizer.hpp"

namespace Common
{

using namespace std;

class BigRamer
{
    string make_L_bigram( const string &left_word, const string &word );
    string make_R_bigram( const string &word, const string &right_word );

    set< string > stopwords;

public:
    BigRamer() {};
    void configure( const string &path_to_stopwords );

    void    bigram2vec( string &a_str, vector< string > &a_res );
    string  bigram2string( string &a_str );
       
};

} // namespace Common

#endif
