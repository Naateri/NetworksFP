#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <string>
#include <vector> 
#include <algorithm>
#include "Node.h"
#include "Client.h"

#define MAX_PCS 2

using namespace std;


class Server {
public:
	Server();
	vector<string> list_nodes;
	vector<int> hash;
	void query(string s);
	int module(int position);
	vector<Node*> nodes;
private:
};

string remove_space(string &s);
#endif

