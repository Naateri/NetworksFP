#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <string>
#include <vector> 
#include <algorithm>

using namespace std;

class Node {
public:
	Node(string val);
	string node_id;
	vector<string> adjacents;
	vector<string> attributes;
	vector<string> values;
	void insert_adj();
	void separate_attributes(string val);
	
	
private:
};

vector<string> separate_string(string s, string delimiter);
string remove_space(string &s);
#endif

