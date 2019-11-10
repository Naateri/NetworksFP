#include "Server.h"

Server::Server() {
	list_nodes.resize(0);
	
}

void Server::query(string s){
	string query = remove_space(s);
	cout<<"QUERY: "<<query<<endl;
	if(query == "INSERT"){
		string node_id = remove_space(s);
		list_nodes.push_back(node_id);
		int act_hash = module(list_nodes.size());
		hash.push_back(act_hash);	
		cout<<" - NAME: "<<node_id<<endl;
		cout<<" - HASH: "<<module(list_nodes.size())<<endl;
		Node* temp = new Node(node_id,s);
		Client* c = new Client(act_hash);
		c->insert_node(temp);
	}
	else if(query == "INSERT_REL"){
		string node1 = remove_space(s);
		string node2 = remove_space(s);
	}
}

int Server::module(int position){
	return position % MAX_PCS;
}

	
string remove_space(string &s){
	string delimiter = " ";
	int pos = s.find(delimiter);
	string s1 = s.substr(0, pos);
	s.erase(0, pos + delimiter.length());
	return s1;
}
