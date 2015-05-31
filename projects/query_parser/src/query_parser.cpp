#include <iostream>

#include "../../../include/common/tokenizer.hpp"
#include "../../../include/common/normalizer.hpp"
#include "../../../include/common/porter2_stemmer.h"
#include "../../../include/common/loc.hpp"
#include "../../../include/common/bigramer.hpp"

using namespace std;

int main()
{
    string str = "   !@a#$%^&*()_-+= a_a `~` `ёёё ййй <>?,./\\\'|\" Раз ДВА    324 !!,..1.1.23 ,,//три    ";
    //boost::tokenizer<> tok(str);
    Common::Tokenizer tok(str);
    vector<string> vec;
    tok.get_vec(vec);
    string here;
    //cout << tok.get_str(":::") << endl;  
 //   cout << tok.get_str(here)  << endl;  
    //string word = "ПоПа";
    //string query = "ПоПа пошла ПОГУЛЯТЬ!!11 ПОПА нашла пирожок ;)";
    string query = "An object of class locale also stores a locale name as an object of class string. Using an invalid locale name to construct a locale facet or a locale object throws an object of class !";
    //Porter2Stemmer::stem(word); 
//    cout << word << endl;
    //Common::init_locale("ru_RU.UTF8");
    Common::init_locale("en_US.UTF-8");
    cout << Common::terminate2str(query) << endl;
    set < string > sss;
    Common::terminate2set(query, sss);
    for (auto s: sss )
        cout << s << endl;
    //Common::init_locale("");
    //Common::init_locale("us_EN.UTF8");
 //   string norm_word = Common::toLower(word);
    
 //   cout << norm_word << endl;

    Common::BigRamer bgr;
    bgr.configure("stopwords.txt");
    cout << "bigramer:\n" << bgr.bigram2string(query) << endl;
    string r_query = "вышел зайчик погулять по луне";
    cout << "bigramer:\n" << bgr.bigram2string(r_query) << endl;
    return 0;
}
