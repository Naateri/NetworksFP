#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <string>
#include <vector> 
#include <algorithm>

using namespace std;

class Node {
public:
	Node(string id,string attrib);
	string node_id;
	vector<string> attributes;
	vector<string> values;
	void separate_attributes(string val);
	
	
private:
};

vector<string> separate_string(string s, string delimiter);

#endif

