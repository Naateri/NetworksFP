/* Server code in C++ */

//g++ server.cpp -o server -std=c++11 -lpthread

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <algorithm>
#include <vector>
#include <fstream>
#include <map>
#include <regex>

#define MAX_SLAVES 5
using namespace std;
int SocketFD, ConnectFD;
char msg[256];
string server_txt = "server.txt";

std::map<int, int> slaves; //Stored as: ID-FD
int node_counter = 0;   

int PORT = 40002;

string slice_string(string &s){
	string delimiter = " ";
	int pos = s.find(delimiter);
	string s1 = s.substr(0, pos);
	s.erase(0, pos + delimiter.length());
	return s1;
}
	
int hash_function(std::string value){
	int cur_sum = 0;
	for (int i = 0; i < value.size(); i++){
		cur_sum += (int)value[i];
	}
	return cur_sum % MAX_SLAVES;
}
		
bool confirming_connection(int ConnectFD){
	char buffer[256];
	int n = read(ConnectFD, buffer, 19);
	if (strcmp("Requesting access.", buffer) != 0){
		n = write(ConnectFD, "No.", 3);
		return false;
	} else {
		n = write(ConnectFD, "OK.", 3);
		bzero(buffer, 256);
		n = read(ConnectFD, buffer, 4);
		return true;
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
				
				
vector<string> separate_attributes(string val){
	std::string delimiter1 = "}";
	vector<string> res;
	
	for (char c: delimiter1) {
		val.erase(std::remove(val.begin(), val.end(), c), val.end());
	}
	if(val.size()==0){
		cout<<"No hay atributos"<<endl;
	}
	else{
		vector<string> valores = separate_string(val,",");
		
		for(int i=0;i<valores.size();++i){
			vector<string> v = separate_string(valores[i],":");
			res.push_back(v[0]);
			res.push_back(v[1]);
		}
	}
	return res;
}
					
					
string lrtrim(string str) {
	const std::string nothing = "" ;
	str = std::regex_replace( str, std::regex( "^\\s+" ), nothing ) ;
	str = std::regex_replace( str, std::regex( "\\s+$" ), nothing ) ;
	return str ; 
}

void delSpaces(string &s){
	s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
}

void insert_node(string &msg){
	vector<string> node = separate_string(msg, "{");
	delSpaces(node[0]);
	string node_id = node[0];
	
	cout<<"Nombre del nodo: |"<< node_id <<"|\n";
	vector<string> attr = separate_attributes(node[1]);
	
	
	for(int i=0;i<attr.size();++i){
		attr[i] = lrtrim(attr[i]);
	}
	
	for(int i=0;i<attr.size();++i){
		cout<<"ATTRIBUTE:|"<<attr[i]<<"|"<<endl;
		cout<<"VALUE:|"<<attr[i+1]<<"|"<<endl;
		++i;
		
	}
	
	int hash = hash_function(node_id);
	/// ------------------
	/// ------------------
	///Cambiar por la comunicacion con el slave
	bool connection = true; 
	/// ------------------
	/// ------------------
	
	if(connection){
		/// Insertar en server.txt
		ofstream fe;
		fe.open(server_txt, ios::app);
		fe.seekp(0, ios::end);
		string node_to_insert =  node_id + ' ' + to_string(hash) + '\n';
		fe << node_to_insert;
		fe.close();	
		
		/// Insertar en el slave
		
		string slave_txt = "slave_" + to_string(hash)+ ".txt";
		cout<<slave_txt<<endl;
		
		ofstream fs;
		fs.open(slave_txt, ios::app | ios::out);
		fs.seekp(0, ios::end);
		fs <<endl;
		fs<<node_id<<endl;
		for(int i=0;i<attr.size();++i){
			fs<<attr[i]<<" : "<<attr[i+1];
			++i;
		}
		
		fs<<endl;
		fs.close();	
	}
	
	else{
		cout<<"Unconnected Slave. Please try later.\n";
	}
}
	
void insert_relation(int num_file, string rel){
	std::fstream myfile, outfile;
	string slave_txt = "slave_" + to_string(num_file)+ ".txt";
	//cout<<slave_txt<<endl;
	myfile.open(slave_txt, ios::in);
	
	string line;

	vector<string> txt;
	txt.push_back(rel);
	cout<<"INSERT_RELATION"<<endl;
	if (myfile.is_open()){
		while ( getline (myfile,line) && !myfile.eof()){
			txt.push_back(line);
		}
		myfile.close();
		
	}
	
	outfile.open(slave_txt, ios::out );
	outfile.clear();
	outfile.seekp(0);
	
	for(int i=0;i<txt.size();++i){
		cout<<txt[i]<<endl;
		outfile <<txt[i];
		outfile<<endl;
	}
	
	outfile.close();
	
}
	
std::string insert(string msg){
	//cout<<msg<<endl;
	bool isNode= false;
	for(int i=0;i<msg.size();++i){
		if(msg[i] == '{'){
			isNode = true;
			break;
		}
		else if(msg[i] == '-'){
			break;
		}
		
	}
	/// Insertando nodo
	if(isNode == true){//////////////
		insert_node(msg);
	}
	/// Insertando relacion
	else{ 
		
		vector<string> nodes = separate_string(msg,"-");
		
		delSpaces(nodes[0]);
		delSpaces(nodes[1]);
		
		cout<<"Nodo 0: "<<nodes[0] <<endl;
		cout<<"Nodo 1: "<<nodes[1] <<endl;
		
		int hash1 = hash_function(nodes[0]);
		int hash2 = hash_function(nodes[1]);

		/// Insertar en server.txt
		ofstream fe;
		fe.open(server_txt, ios::app);
		fe.seekp(0, ios::end);
		fe << nodes[0] + " " +to_string(hash1)<<endl;
		fe << nodes[1] + " " +to_string(hash2)<<endl;
		fe.close();	

		
		string rel1 =nodes[0] +" - "+nodes[1];
		string rel2 =nodes[1] +" - "+nodes[0];
		
		insert_relation(hash1, rel1);
		insert_relation(hash2, rel2);
	}
	
	
	
	
	std::string temp(msg); //communicate with appropiate slave
	std::string return_to_client; //result to be sent to client
	//put in temp the node value
	int hash = hash_function(temp);
	
	//comunicate with appropiate slave
	
	return return_to_client;
}
	
	
std::string select_node(const char* msg, int lvl){
	std::string temp(msg); //communicate with appropiate slave
	std::string return_to_client; //result to be sent to client
	//put in temp the node value
	int hash = hash_function(temp);
	cout<<lvl<<endl;
	hash++;
	std::fstream file;
	string slave_txt = "slave_" + to_string(hash)+ ".txt";
	//cout<<slave_txt<<endl;
	file.open(slave_txt, ios::in);
	
	string line,res;
	vector<string> separate = separate_string(temp, " ");
	bool attributes = 0;
	bool findAttibutes = 0;
	bool once = 1;
	if (file.is_open()){
		cout<<"Select information"<<endl;
		if(stoi(separate[separate.size()-1]) == 1){
			while ( getline (file,line) && !file.eof()){
				
				if(line == "" && once){
					
					res+= "Attributes: ";
					attributes = 1;
					once = 0;
				}
				if(!attributes){
					if(separate[0][0] == line[0]){
						res+= line + " == ";
					}
				}else{
					if(separate[0][0] == line[0]){
						
						findAttibutes = 1;
					}
					if(findAttibutes){
						if(line == ""){
							break;
						}
						
						res+=line+" == ";
					}
				}
				
			}
			return_to_client = res;
			file.close();
		}
		
	}
	
	
	//comunicate with appropiate slave
	
	return return_to_client;
}
		
std::string parse_message(char msg[256]){
	int n = 0, level; //n = message type
	string str_msg(msg, strlen(msg));
	
	string query = slice_string(str_msg);
	transform(query.begin(), query.end(),query.begin(), ::tolower);
	cout<<query<<endl;
	
	
	//know what the server wants
	if (query == "insert" ){
		return insert(str_msg);
	} else if (query == "select"){
		cout<<"Entro Select"<<endl;
		return select_node(str_msg.c_str(), level);
	} else {
		return "Error. Query not understood\n";
	}
}
			
void rcv_msg(int ConnectFD){
	char buffer[256];
	std::string slave_msg;
	bool msg_from_slave = false;
	std::string result;
	int n;
	do{
		bzero(buffer,256);
		n = read(ConnectFD,buffer,255);
		std::string temp(buffer, 256);
		if (n < 0) perror("ERROR reading from socket");
		printf("Client %d: [%s]\n", ConnectFD, buffer);
		
		if (temp.substr(0, 9) == "SLAVE_MSG"){
			msg_from_slave = true;
		} else msg_from_slave = false;
		
		if (!msg_from_slave){
			result = parse_message(buffer);
			n = write(ConnectFD, result.c_str(), result.size());
		}
		
	}while ( strcmp(msg, "chau") != 0);
	
	shutdown(ConnectFD, SHUT_RDWR);
	
	close(ConnectFD);
}
				
int main(void) {
	struct sockaddr_in stSockAddr;
	SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	int Res;
	
	if(-1 == SocketFD)
	{
		perror("can not create socket");
		exit(EXIT_FAILURE);
	}
	
	memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
	
	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(PORT);
	stSockAddr.sin_addr.s_addr = INADDR_ANY;
	
	if(-1 == bind(SocketFD,(const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in))){
		perror("error bind failed");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}
	
	if(-1 == listen(SocketFD, 10)){
		perror("error listen failed");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}
	
	for(;;){
		ConnectFD = accept(SocketFD, NULL, NULL);
		
		if(0 > ConnectFD){
			perror("error accept failed");
			close(SocketFD);
			exit(EXIT_FAILURE);
		}
		
		bool connection = confirming_connection(ConnectFD);
		if (connection){
			/* perform read write operations ... */
			std::thread t1(rcv_msg, ConnectFD);
			t1.detach();
		} else {
			shutdown(ConnectFD, SHUT_RDWR);
			close(ConnectFD);
		}
	}
	
	close(SocketFD);
	return 0;
}
