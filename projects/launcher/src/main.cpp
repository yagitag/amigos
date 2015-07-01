#include <iostream>
#include <string>
#include "../../../include/launcher/launcher.hpp"
#include "../../../include/searcher/searcher.hpp"

using namespace std;

int main(int argc, char *argv[]) 
{
    if( argc != 4 )
    {
        std::cerr << "USAGE: " << string(argv[0]) << " path_to_config path_to_stopwords query" << std::endl;
        return 1;
    }
    Launcher launcher;
    launcher.configure(string(argv[1]), string(argv[2]));

    vector<Document> docs;
    string query = string(argv[3]);
    launcher.launch_searcher( query, docs );

    std::cout << docs.size() << std::endl;   

    return 0;
}
