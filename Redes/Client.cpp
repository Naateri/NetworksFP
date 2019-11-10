#include "Client.h"

Client::Client(int hash) {
	file = "client"+to_string(hash)+".txt";
}

void Client::insert_node(Node* tmp){
	ofstream fe;
	fe.open(file, ios::app);
	fe.seekp(0, ios::end);
	fe<<tmp->node_id<<endl;
	for(int i=0;i<tmp->attributes.size();++i){
		fe<<tmp->attributes[i]<<" : "<<tmp->values[i]<<endl;
	}

	fe<<endl;
	fe.close();	
}
void Client::insert_relation(pair<string,string> rel){
	
}

