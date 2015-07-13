#include <iostream>
#include <string>
#include "../../../include/launcher/launcher.hpp"
#include "../../../include/searcher/searcher.hpp"

using namespace std;

int main(int argc, char *argv[])
{
    if( argc != 3 )
    {
        std::cerr << "USAGE: " << string(argv[0]) << " path_to_config path_to_stopwords" << std::endl;
        return 1;
    }
    Launcher launcher;
    launcher.configure(string(argv[1]), string(argv[2]));

    cout << "CONFIGURED!" << std::endl;
    while(1)
    {
        cout << "Query: ";
        string query;
        getline(cin, query);

        vector<Document> docs;
        vector<uint32_t> docsId;
        //string query = string(argv[3]);
        launcher.launch_searcher( query, docsId );

        for(size_t i = 0; i < 10; ++i)
        {
            Document &doc = docs[i];
            std::vector< Snippet > snippets;
            launcher.get_snippets(query, doc.docId, snippets, 3);
            cout << "SNIPPETS:" << endl;
            for( Snippet &spt : snippets )                                         
            {                                                                      
                cout << "SUBTITLE: " << spt.subtitle.first << std::endl;           
                for( auto sel : spt.selections )                                   
                {                                                                  
                    cout << "SEL: " << sel.first << " SIZE: " << sel.second << endl;
                }                                                                  
            }                                                                      

        }

        std::cout << "DOCS NUM FROM LAUNCHER: " << docs.size() << std::endl;
    }

    return 0;
}
