#ifndef __NORMALIZER__HPP__
#define __NORMALIZER__HPP__

#include <string>
#include <locale>
#include <cstdlib>
#include <vector>
#include <string>
#include <set>

#include <algorithm>
#include <functional>

#include "loc.hpp"
#include "porter2_stemmer.h"
#include "tokenizer.hpp"

namespace Common {

using namespace std;

string toLower( const string &str )
{
    assert( is_locale_init() );

    wchar_t ws_c[str.size()];
    mbstowcs(ws_c, str.c_str(), str.size()+1);
    wstring ws(ws_c);
    transform(ws.begin(), ws.end(), ws.begin(), bind2nd(ptr_fun(&std::tolower<wchar_t>), locale()));
    char low_str_c[str.size()];
    wcstombs(low_str_c, ws.c_str(), str.size()+1);
    string low_str(low_str_c);

    return low_str;
}

string normalize_word( const string &word )
{
    string norm_word = toLower(word);
    Porter2Stemmer::stem(norm_word);
    return norm_word;
}

void terminate2vec( const string &str, vector < string > &vec )
{
    Tokenizer tkns(str);
    tkns.get_vec(vec);
    for (string &term: vec)
        term = normalize_word(term);
}

void terminate2set( const string &str, set < string > &res_set )
{
    Tokenizer tkns(str);
    set < string > tmp_set;
    tkns.get_set(tmp_set);
    for (string term: tmp_set)
    {
        term = normalize_word(term);
        res_set.insert(term);
    }
}

string terminate2str( const string &str, const string &delim = " " )
{
    Tokenizer tkns(str);
    vector < string > vec;
    tkns.get_vec(vec);
    string res;
    for (string &term: vec)
        term = normalize_word(term);
    return boost::algorithm::join(vec, delim);
}

} //namespca Common

#endif
