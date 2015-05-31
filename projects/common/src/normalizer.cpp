#include "../../../include/common/normalizer.hpp"

#include <string>
#include <locale>
#include <cstdlib>
#include <vector>
#include <string>
#include <set>

#include <algorithm>
#include <functional>
#include <boost/algorithm/string.hpp>

#include "../../../include/common/loc.hpp"
#include "../../../include/common/porter2_stemmer.h"
#include "../../../include/common/tokenizer.hpp"

namespace Common {

using namespace std;

wstring str2wstr( const string &str )
{
    wchar_t ws_c[str.size()];
    mbstowcs(ws_c, str.c_str(), str.size()+1);
    return wstring(ws_c);
}


string wstr2str( const wstring &ws, size_t a_size )
{
    char low_str_c[a_size];
    wcstombs(low_str_c, ws.c_str(), a_size+1);
    return string(low_str_c);
}

string toLower( const string &str )
{
    assert( is_locale_init() );

    wstring ws = str2wstr( str );
    transform(ws.begin(), ws.end(), ws.begin(), bind2nd(ptr_fun(&std::tolower<wchar_t>), locale()));
    string low_str = wstr2str(ws, str.size());

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

string terminate2str( const string &str, const string &delim)
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
