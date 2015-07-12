#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <fstream>
#include <sstream>
#include "launcher.hpp"
#include <wchar.h>
#include <cstring>

#define MY_PORT    5000

//сервер
const int SOCKET_ERROR = -1;
using namespace std;

Launcher launcher;

string ReplaceSubstring(string str, string replaceStr, string newStr)
{
	size_t index = 0;
	while (true) 
	{
     
     index = str.find(replaceStr, index);
     if (index == string::npos) 
		 break;

     str.replace(index, replaceStr.size(), newStr);

     index += replaceStr.size();
	}
	return replaceStr;
}

string Format(string str)
{
	string symbols [] = {"&", "<", ">", "\"", "\'"};
	string symbolCode [] = {"&amp;", "&lt;", "&gt;", "&quot;", "&apos;"};

	for (size_t i = 0; i < 5; i++)
		str = ReplaceSubstring(str, symbols[i], symbolCode[i]); 
	return str;
}

string MakeXml(std::vector<Document> &docs, string query, int startDoc, int endDoc, bool findOnlyInSubs)
{
	string xml = "";
	xml += "<?xml version=\"1.0\" encoding=\"utf-16\"?>\n";
	std::string count = std::to_string((long long)docs.size());
	xml += "<results totalResultsCount = \"" + count + "\">\n";

	for (int i = startDoc; i < endDoc && i < docs.size(); i++)
	{
		xml += "\t <result link=\"https://www.youtube.com/watch?v=" + docs[i].videoId + "\" ";
		xml += "title=\"" + docs[i].title + "\" ";
		xml += "description=\"\" ";
		//xml += "selectionStart=\"0\" selectionLength=\"0\" ";
		xml += "> \n";

		std::vector< Snippet > snippets;
		uint32_t snippets_num = 3;

		launcher.get_snippets(query, docs[i].docId, snippets, snippets_num);
		for (size_t subNum = 0; subNum < snippets.size(); subNum++)
		{
			xml += "\t\t <snippet text=\"" + Format(snippets[subNum].subtitle.first) + "\" ";
			xml += "time=\"" + std::to_string((long long)snippets[subNum].subtitle.second) + "\" ";
			xml += ">\n";

			for (size_t pos = 0; pos < snippets[subNum].selections.size(); pos++)
			{
				xml += "\t\t\t <selection start=\"" + std::to_string((long long)snippets[subNum].selections[pos].first) + "\" ";
				xml += "lenght=\"" + std::to_string((long long)snippets[subNum].selections[pos].second) + "\"/>\n";
			}
			xml += "\t\t</snippet>\n";
		}

		xml += "\t </result>";
	}

	xml += "</results>";
	return xml;
}

void ParseQuery(string &query, string &toSearch, int &startDoc, int &endDoc, bool &findOnlyInSubs)
{
	int start = 0, end = 0;
	startDoc = 0; endDoc = 0;
	
	for (size_t i = 0; i < query.length(); i++)
	{
		if (query[i] == '\"')
		{
			start = i;
			break;
		}
	}

	for (int i = query.length()-1; i >= 0; i--)
	{
		if (query[i] == '\"')
		{
			end = i;
			break;
		}
	}

	toSearch = query.substr(start+1, end - start-1);

	int i = end+1;
	while (query[i] == ' ')
		i++;

	while (query[i] != ' ')
	{
		startDoc = 10 * startDoc + query[i] - '0';
		i++;
	}

	while (query[i] == ' ')
		i++;

	while (query[i] != ' ')
	{
		endDoc = 10 * endDoc + query[i] - '0';
		i++;
	}

	while (query[i] == ' ')
		i++;
	
	if (query[i] == 't')
		findOnlyInSubs = true;
	else
		findOnlyInSubs = false;
}

string Search(int bytes_recv, char *buff)
{
	string answer;
	char * str = (char *)malloc(bytes_recv + 1);
	memcpy(str, buff, bytes_recv);
	str[bytes_recv] = '\0';
	string query = "";
	printf("Receved: %d\n", bytes_recv);
	int i = 0;
	while (i < bytes_recv)
	{
		if (str[i] != '\0')
			query.push_back(str[i]);
		i++;
	}
	query.push_back('\0');
	printf("%s\n", query.c_str());

	//MessageBox(0, query.c_str(), query.c_str(), MB_OK);

	string toSearch;
	int startDoc;
	int endDoc;
	bool findOnlyInSubs;

	ParseQuery(query, toSearch, startDoc, endDoc, findOnlyInSubs);


	std::vector<Document> docs;
	launcher.launch_searcher(toSearch, docs);
	answer = MakeXml(docs, toSearch, startDoc, endDoc, findOnlyInSubs);

	free(str);
	return answer;
}


wchar_t* ConvertToWideChar(char* p)// function which converts char to widechar
{
  wchar_t *r;
  r = new wchar_t[strlen(p)+1];

  char *tempsour = p;
  wchar_t *tempdest = r;
  while(*tempdest++=*tempsour++);

  return r;
}


string TestXml()
{
	string line;
	string text;
	int count = 0;
	ifstream myfile("true_xml.txt");

	if (myfile.is_open())
	{
		while (!myfile.eof())
		{
			getline(myfile, line);
			text += line;
		}
		myfile.close();
	}
	else cout << "Unable to open file";

	for (string::iterator i = text.begin(); i != text.end(); i++)
		if (*i == ',')
			count++;

	
	cout << "Count = " << count << endl;

	return text;
}

int main(int argc, char* argv[])
{

	char buff[1024]; 

  if (argc != 3) {
    std::cerr << "Usage: lister <path_to_config> <path_to_stopwords>" << std::endl;
    return 1;
  }
	const std::string path_to_config = argv[1];
	const std::string path_to_stopwords = argv[2];

	//Launcher launcher;

	launcher.configure(path_to_config, path_to_stopwords);

	int   mysocket,  client_socket;
	struct sockaddr_in    local_addr, client_addr;
	int client_addr_size=sizeof(client_addr);

	if ((mysocket=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		printf("Error socket ");
		return -1;
	}

	local_addr.sin_family=AF_INET;
	local_addr.sin_port=htons(MY_PORT);
	local_addr.sin_addr.s_addr = 0;

	if (bind(mysocket,(struct sockaddr *) &local_addr,
		sizeof(local_addr))) {
			printf("Error bind ");
			close(mysocket);
			return -1;
	}

	if (listen(mysocket, 0x100))
	{
		printf("Error listen ");
		close(mysocket);
		return -1;
	}
	printf("Waiting for calls\n");

	while((client_socket=accept(mysocket, (struct sockaddr *)
		&client_addr, (unsigned int*)&client_addr_size)))
	{
		struct hostent *hst;
		int bytes_recv;
		hst=gethostbyaddr((char *)&client_addr.sin_addr.s_addr,4, AF_INET);
		printf("+%s [%s] new connect!\n",
			(hst)?hst->h_name:"",  inet_ntoa(client_addr.sin_addr));

		while ((bytes_recv = recv(client_socket, &buff[0], sizeof(buff), 0))
			&& bytes_recv != SOCKET_ERROR)
		{
			string text = Search(bytes_recv, buff);

			send(client_socket, text.c_str(), text.size() * sizeof(char), 0);
		}

		printf("Client was disconnected\n"); 
		close(client_socket);
	}

	return 0;
}
