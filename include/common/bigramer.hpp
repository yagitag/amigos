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

    void    bigram2vec( vector< string > &words_for_bigraming, vector< string > &a_result );
    void    bigram2vec( string &string_for_bigraming, vector< string > &a_result );
    string  bigram2string( vector< string > &words_for_bigraming );
    string  bigram2string( string &string_for_bigraming );
       
};

} // namespace Common

#endif
