#ifndef __TOKENIZER__HPP__
#define __TOKENIZER__HPP__

#include<string>
#include<set>

#include<boost/tokenizer.hpp>
#include<boost/algorithm/string.hpp>

namespace Common {

using namespace std;

class Tokenizer : public boost::tokenizer<>
{
public:
    template <typename Container> Tokenizer(const Container &c): 
        tokenizer<>::tokenizer(c) {}

    void get_vec( vector < string > & );
    void get_set( set < string > & );
    string get_str( const string &delim = " " );
};

/** 
Also, you can get tokens like this:

    Tokenizer tok(str);                                           
    for(Tokenizer::iterator beg=tok.begin(); beg!=tok.end();++beg)
        cout << *beg << endl;                                      
**/

void Tokenizer::get_vec( vector < string > &vec )
{
    for (const string &token: *this)
        vec.push_back(token);
}
void Tokenizer::get_set( set < string > &res_set )
{
    for (const string &token: *this)
        res_set.insert(token);
}
string Tokenizer::get_str( const string &delim )
{
    return boost::algorithm::join(*this, delim);
}

} // Core namespace

#endif
