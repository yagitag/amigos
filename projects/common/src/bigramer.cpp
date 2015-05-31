#include "../../../include/common/bigramer.hpp"

#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include "../../../include/common/normalizer.hpp"
#include "../../../include/common/tokenizer.hpp"

namespace Common
{

using namespace std;

string BigRamer::make_L_bigram( const string &left_word, const string &word )
{
    wstring w_left_word = str2wstr( left_word );
    wstring w_word = str2wstr( word );
    
    assert(w_left_word.size());
    assert(w_word.size());

    wstring w_res;
    size_t l_w_size = w_left_word.size();
    if (l_w_size > 1)
        w_res = wstring(L"_").append(&w_left_word[l_w_size-2]).append(w_word);
    else
        w_res = wstring(L"__").append(&w_left_word[l_w_size-1]).append(w_word);


    return wstr2str( w_res, word.size()+6 );
}

string BigRamer::make_R_bigram( const string &word, const string &right_word )
{
    wstring w_right_word = str2wstr( right_word );
    wstring w_word = str2wstr( word );
    
    assert(w_right_word.size());
    assert(w_word.size());

    wstring w_res;
    size_t r_w_size = w_right_word.size();
    if (r_w_size > 1)
    {
        wstring right_part = L"";
        right_part.push_back(w_right_word[0]);
        right_part.push_back(w_right_word[1]);
        w_res = wstring(w_word).append(right_part).append(L"_");
    }
    else
    {
        wstring right_part = L"";
        right_part.push_back(w_right_word[0]);
        w_res = wstring(w_word).append(&w_right_word[r_w_size-1]).append(L"__");
    }

    return wstr2str( w_res, word.size()+6 );
}

void BigRamer::configure( const string &path_to_stopwords )
{
    ifstream stopwords_f(path_to_stopwords.c_str());
    if( !stopwords_f )
    {
        cerr << "ERROR: some shit with stopwords file: " << path_to_stopwords << endl;
        return;
    }
    stopwords.clear();
    while( stopwords_f )
    {
        string word;
        stopwords_f >> word;
        if( word.size() )
            stopwords.insert(word);
    }
}

void BigRamer::bigram2vec( string &a_str, vector< string > &a_res )
{
    Tokenizer tkn( a_str );
    vector < string > words;
    tkn.get_vec(words);
    
    for( vector < string >::const_iterator it = words.begin(); it != words.end(); ++it )
    {
        if( find(stopwords.begin(), stopwords.end(), *it) != stopwords.end() )
        {
            if( it != words.begin() )
            {
                string bigram = make_L_bigram(*(it-1), *it);
                a_res.push_back( bigram );
            }
            if( it+1 != words.end() )
            {
                string bigram = make_R_bigram(*it, *(it+1));
                a_res.push_back( bigram );
            }
        }
        else 
            a_res.push_back( *it );
    }
}

string BigRamer::bigram2string( string &a_str )
{
    vector< string > res_vec;
    bigram2vec(a_str, res_vec);
    return boost::algorithm::join(res_vec, " ");
}

} // namespace Common
