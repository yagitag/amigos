#include "../../../include/common/tokenizer.hpp"

#include<string>
#include<set>
#include<vector>

#include<boost/tokenizer.hpp>
#include<boost/algorithm/string.hpp>


namespace Common {

using namespace std;

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
