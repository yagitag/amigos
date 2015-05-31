#ifndef __TOKENIZER__HPP__
#define __TOKENIZER__HPP__

#include<string>
#include<set>
#include<vector>

#include<boost/tokenizer.hpp>

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

} // Core namespace

#endif
