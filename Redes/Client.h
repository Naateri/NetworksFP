#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <string>
#include <vector> 
#include <algorithm>
#include <fstream>
#include "Node.h"


class Client {
public:
	Client(int hash);
	int hash;
	void insert_node(Node* tmp);
	void insert_relation(pair<string,string> rel);
	string file;
private:
};

#endif

