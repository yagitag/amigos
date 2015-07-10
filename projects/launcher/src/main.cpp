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
        //string query = string(argv[3]);
        launcher.launch_searcher( query, docs );

        std::cout << "DOCS NUM FROM LAUNCHER: " << docs.size() << std::endl;
    }

    return 0;
}
