#include <iostream>
#include "../../../include/launcher/launcher.hpp"
#include "../../../include/searcher/searcher.hpp"

using namespace std;

int main(int argc, char *argv[]) 
{
    Launcher launcher;
    launcher.configure(string(argv[1]), "");

    vector<Document> docs;
    string query = string(argv[2]);
    launcher.launch_searcher( query, docs );

    std::cout << docs.size() << std::endl;   

    return 0;
}
