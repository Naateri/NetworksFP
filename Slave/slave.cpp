/* Client code in C++ */

//g++ client.cpp -o client -std=c++11 -lpthread

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
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <regex>

#define MAX_SLAVES 5
#define uint unsigned int

using namespace std;

int SocketFD;
char buffer[256];

std::string IP = "127.0.0.1";
int PORT = 40004;

bool end_connection = false;

/*void requesting_access(int SocketFD, string identificador){
	string request="Slave requesting access "+identificador;
	int n = write(SocketFD, "Slave requesting access", 26);
	
	n = read(SocketFD,buffer,255);
	
	if (strcmp(buffer, "OK.") != 0){
		printf("Erroneous confirmation. Ending connection\n");
		shutdown(SocketFD, SHUT_RDWR);
		close(SocketFD);
	} else {
		n = write(SocketFD, "OK.", 3);
		printf("Connection to database established. SLAVE.\n");
	}
}*/
void delSpaces(string &s){
	s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
}	
	
string slice_string(string &s){
	string delimiter = " ";
	int pos = s.find(delimiter);
	string s1 = s.substr(0, pos);
	s.erase(0, pos + delimiter.length());
	return s1;
}
	
vector<string> separate_string(string s, string delimiter){
	vector<string> values;
	uint pos = s.find(delimiter);
	
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
	std::string delimiter1 = "{}";
	vector<string> res;
	
	for (char c: delimiter1) {
		val.erase(std::remove(val.begin(), val.end(), c), val.end());
	}
	if(val.size()==0){
		cout<<"No attributes"<<endl;
	}
	else{
		vector<string> valores = separate_string(val,",");
		
		for(uint i=0;i<valores.size();++i){
			vector<string> v = separate_string(valores[i],":");
			res.push_back(v[0]);
			res.push_back(v[1]);
		}
	}
	return res;
}
	
int hash_function(std::string value){
	int cur_sum = 0;
	for (uint i = 0; i < value.size(); i++){
		cur_sum += (int)value[i];
	}
	return cur_sum % MAX_SLAVES + 1;
}
	
string lrtrim(string str) {
	const std::string nothing = "" ;
	str = std::regex_replace( str, std::regex( "^\\s+" ), nothing ) ;
	str = std::regex_replace( str, std::regex( "\\s+$" ), nothing ) ;
	return str ; 
}

int get_id(){
	ifstream fs;
	fs.open("slave.txt");
	
	string line, value;
	value.clear();
	if (fs.is_open()){
		getline(fs, line);
		int i = 0;
		while(line[i] != '-'){ //mejorar pls
			value += line[i++];
		}
		if (value.size() == 0) return 0;
		
		//FALTA CONSIDERAR ESPACIOS
		
		return hash_function(value);
	} else return 0;
}

string insert_node(string s){
	
	std::string return_to_server = "slave Node was inserted"; //result to be sent to client
	string node_id = slice_string(s);
	//cout<<"NODE ID: " <<node_id<<endl;
	//cout<<"RESTO: "<<s<<endl;
	vector<string> attr;
	
	if(s.size()>5){/// 2 minimo {a:c}
		attr = separate_attributes(s);
		for(uint i=0;i<attr.size();++i){
			attr[i] = lrtrim(attr[i]);
			//cout<<"|"<<attr[i]<<"|"<<endl;
		}
	}
	else{
		attr.resize(0);
	}
	
	
	
	/*
	for(uint i=0;i<attr.size();++i){
		cout<<"ATTRIBUTE:|"<<attr[i]<<"|"<<endl;
		cout<<"VALUE:|"<<attr[i+1]<<"|"<<endl;
		++i;
		
	}
	*/
	
	int hash = hash_function(node_id);
	
	
	///string slave_txt = "slave_" + to_string(hash)+ ".txt";
	
	string slave_txt = "slave.txt";
	
	//cout<<slave_txt<<endl;
	ofstream fs;
	fs.open(slave_txt, ios::app);
	fs.seekp(0, ios::end);
	if(fs.is_open()){
		
		fs <<endl;
		fs<< node_id<<endl;
		for(uint i=0;i<attr.size();i+=2){
			//cout<<"I: "<<i<<endl;
			fs<<attr[i]<<" : "<<attr[i+1]<<endl;
		}
	}
	

	fs.close();	
	
	return return_to_server;
	
}
	
string insert_adjacency(string s){
	vector<string> nodes = separate_string(s , " ");
	
	
	string rel =nodes[0] +" - "+nodes[1];

	int num_file = hash_function(nodes[0]);
	

	
	std::fstream myfile, outfile;
	
	///string slave_txt = "slave_" + to_string(num_file)+ ".txt";
	
	string slave_txt = "slave.txt";
	
	//cout<<slave_txt<<endl;
	myfile.open(slave_txt, ios::in);
	
	string line;
	
	vector<string> txt;
	txt.push_back(rel);
	//cout<<"INSERT_RELATION"<<endl;
	bool isInserted = false;
	if (myfile.is_open()){
		while ( getline (myfile,line) && !myfile.eof()){
			txt.push_back(line);
			if(line == nodes[0]){
				isInserted = true;
			}
		}
		myfile.close();
		
	}
	

	
	outfile.open(slave_txt, ios::out );
	outfile.clear();
	outfile.seekp(0);
	
	for(uint i=0;i<txt.size();++i){
		//cout<<txt[i]<<endl;
		outfile <<txt[i]<<endl;
	}
	
	outfile.close();
	
	if(!isInserted){
		string toInsert = nodes[0] + " {}";
		insert_node(toInsert);
	}
	
	return "slave Adjacency was inserted";
}
	
string all_adjacencies(string msg){
	delSpaces(msg);
	string node_id = msg;
	int num_file = hash_function(msg);
	
	std::fstream myfile;
	
	//string slave_txt = "slave_" + to_string(num_file)+ ".txt";
	string slave_txt = "slave.txt";
	
	myfile.open(slave_txt, ios::in);
	
	string line;
	
	vector<string> txt;
	txt.push_back(node_id);
	//cout<<"INSERT_RELATION"<<endl;
	bool isInserted = false;
	if (myfile.is_open()){
		while ( getline (myfile,line) && !myfile.eof()){
			if(line == "\0"){
				break;
			}
			
			size_t found = line.find(node_id);
			if (found != string::npos) 
				txt.push_back(line);
			
		}
		myfile.close();
		
	}
	
	for(int i=0;i<txt.size();++i){
		cout<<txt[i]<<endl;
	}
	
	/// Utilizar un set
	///Falta terminar	
	
	return "slave hola" ;
	
}
	
string delete_adjacency(string s){
	vector<string> nodes = separate_string(s , " ");
	
	
	string rel =nodes[0] +" - "+nodes[1];
	
	int num_file = hash_function(nodes[0]);
	
	
	
	std::fstream myfile, outfile;
	
	//string slave_txt = "slave_" + to_string(num_file)+ ".txt";
	
	string slave_txt = "slave.txt";
	
	//cout<<slave_txt<<endl;
	myfile.open(slave_txt, ios::in);
	
	string line;
	
	vector<string> txt;

	if (myfile.is_open()){
		while ( getline (myfile,line) && !myfile.eof()){
		
			if(line != rel){
				txt.push_back(line);
			}
			
		}
		myfile.close();
	}
	
	outfile.open(slave_txt, ios::out );
	outfile.clear();
	outfile.seekp(0);
	
	for(uint i=0;i<txt.size();++i){
		//cout<<txt[i]<<endl;
		outfile <<txt[i]<<endl;
	}
	
	outfile.close();
	
	return "slave Adjacency was deleted";
}
	
std::string parse_message(string msg){
	string res;
	
	string type_query = slice_string(msg);
	transform(type_query.begin(), type_query.end(),type_query.begin(), ::tolower);
	
	
	if(type_query == "0"){
		cout<<"Inserting node"<<endl;
		res = insert_node(msg);
	}
	else if(type_query == "1"){
		cout<<"Inserting adjacency"<<endl;
		res = insert_adjacency(msg);
	}
	else if(type_query == "2"){
		cout<<"Select"<<endl;
		std::fstream file;
		string slave_txt = "slave.txt";
		//res = select(msg);
		file.open(slave_txt, ios::in);
		
		string line,res;
		vector<string> separate = separate_string(msg, " ");
		vector<string> nodes;
		nodes.push_back(separate[separate.size()-2]);
		bool attributes = 0;
		bool findAttibutes = 0;
		bool once = 1;
		if (file.is_open()){
			cout<<"Select information"<<endl;
			cout<<separate[separate.size()-1][1]<<endl;
			cout<<separate[separate.size()-1][0]<<endl;
			string tempLevelString(1,separate[separate.size()-1][1]);
			if(stoi(tempLevelString) == 1){
				while ( getline (file,line) && !file.eof()){
					
					if(line == "" && once){
						
						res+= "Attributes: ";
						attributes = 1;
						once = 0;
					}
					if(!attributes){
						if(separate[separate.size()-1][0] == line[0]){
							res+= line + " == ";
							vector<string> temp = separate_string(line, " ");
							nodes.push_back(temp[temp.size()-1]);
						}
					}else{
						for(uint i = 0;i<nodes.size();++i){
							if(nodes[i][0] == line[0] && line != ""){
								findAttibutes = 1;
								break;
							}
						}
						if(findAttibutes){
							res+=line+" == ";
							findAttibutes =0;
						}
					}
					
				}
				string lengthString = to_string(res.size());
				while(lengthString.size()<6){
					lengthString = "0"+lengthString;
				}
				res =  "s"+lengthString+res;
				cout<<"Res" << res<<endl;
				return res;
				file.close();
			}
			
		}
		
	} 
	else if(type_query == "3"){
		cout<<"Delete node"<<endl;
		//res = delete_node(msg);
	} 
	else if(type_query == "adj"){
		cout<<"Requesting all adjacencies of a node"<<endl;
		res = all_adjacencies(msg);
	}
	else if(type_query == "4"){
		cout<<"Delete adjacency"<<endl;
		res = delete_adjacency(msg);
	} 
	else {
		res = "Error. Query not understood\n";
	}
	
	return res;
	
}

void requesting_access(int SocketFD, string identificador){ //new requesting access
	string request="Slave requesting access "+identificador;
	write(SocketFD, "Slave requesting access", 23+1);
	
	sleep(1);
	
	write(SocketFD, identificador.c_str(), identificador.size());
	
	read(SocketFD,buffer,255);
	
	if (strcmp(buffer, "OK.") != 0){
		printf("Erroneous confirmation. Ending connection\n");
		shutdown(SocketFD, SHUT_RDWR);
		close(SocketFD);
	} else {
		write(SocketFD, "OK.", 3);
		printf("Connection to database as a slave established.\n");
	}
}
	
void send_msg(){
	char msg[256];
	int n;
	do{
		bzero(msg,256);
		std::cout << "Type your message: ";
		std::cin.getline(msg, 255);
		n = write(SocketFD,msg,255); //cuantos bytes estoy mandando
		
		//n dice cuantos bytes se han mandado
		msg[n] = '\0';
	} while(!end_connection);
}
		
void rcv_msg(){
	char buffer[256];
	int n;
	do{	
		bzero(buffer,256);
		n = read(SocketFD,buffer,255);
		
		std::string temp(buffer, 256);
		cout<<"temp: "<<temp<<endl;
		std::size_t found = temp.find('\0');
	
		temp = temp.substr(0,found);
	
		string result;
		if (n < 0) perror("ERROR reading from socket");
		
		if (temp.substr(0, 6) == "server"){
			
			slice_string(temp);
			result = parse_message(temp);
			cout<<"Resultado Select: "<<result<<endl;
			n = write(SocketFD, result.c_str(), result.size());
			

		}
		
		

		printf("Server: [%s]\n",buffer);
	} while(!end_connection);
	
	end_connection = true;
}
			
int main(void){
	cout << "SLAVE"<<endl;
	struct sockaddr_in stSockAddr;
	int Res;
	SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	//char msg[256];
	
	if (-1 == SocketFD)
	{
		perror("cannot create socket");
		exit(EXIT_FAILURE);
	}
	
	memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
	
	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(PORT); //port
	Res = inet_pton(AF_INET, IP.c_str(), &stSockAddr.sin_addr);
	
	if (0 > Res)
	{
		perror("error: first parameter is not a valid address family");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}
	else if (0 == Res)
	{
		perror("char string (second parameter does not contain valid ipaddress");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}
	//hacer un chat para la siguiente clase
	int cnct = connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));
	if (-1 == cnct) {
		perror("connect failed");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}
	/*do{
	}while(strcmp(buffer, "chau") != 0); */
	
	string id = to_string(get_id());
	
	requesting_access(SocketFD, id);
	
	//std::thread t1(send_msg);
	std::thread t2(rcv_msg);
	
	//t1.join();
	t2.join();
	shutdown(SocketFD, SHUT_RDWR);
	
	close(SocketFD);
	
	return 0;
}
	
