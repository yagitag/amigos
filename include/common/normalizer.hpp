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

wstring str2wstr( const string &str );

string wstr2str( const wstring &ws, size_t a_size );

string toLower( const string &str );

string normalize_word( const string &word );

void terminate2vec( const string &str, vector < string > &vec );

void terminate2set( const string &str, set < string > &res_set );

string terminate2str( const string &str, const string &delim = " " );

} //namespca Common

#endif
