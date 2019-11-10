#include "Node.h"

Node::Node(string id, string attrib) {
	this->node_id = id;
	separate_attributes(attrib);
}

void Node::separate_attributes(string val){
	std::string delimiter1 = "{}";
	
	for (char c: delimiter1) {
		val.erase(std::remove(val.begin(), val.end(), c), val.end());
	}
	
	vector<string> valores = separate_string(val,",");

	for(int i=0;i<valores.size();++i){
		vector<string> v = separate_string(valores[i],":");
		attributes.push_back(v[0]);
		values.push_back(v[1]);
	}
		
	for(int i=0;i<attributes.size();++i){
		cout<<i<<endl;
		cout<<"ATTRIBUTE:"<<attributes[i]<<endl;
		cout<<"VALUE:"<<values[i]<<endl;
	}
	
}

vector<string> separate_string(string s, string delimiter){
	vector<string> values;
	int pos = s.find(delimiter);
	
	while (pos < s.size()) {
		string s1 = s.substr(0, pos);
		values.push_back(s1);
		s.erase(0, pos + delimiter.length());
		pos = s.find(delimiter);
	}
	values.push_back(s);
	return values;
}
	
