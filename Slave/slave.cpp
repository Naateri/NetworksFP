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

#define MAX_SLAVES 2
#define uint unsigned int

using namespace std;

int SocketFD;
char buffer[256];

std::string IP = "127.0.0.1";
int PORT = 40005;

bool end_connection = false;
const int l = 3;




string size_string(string s){
	int num = s.size();
	num += l+1;
	string res = to_string(num);
	
	if(res.size() == 1)
		res = "00"+res;	 
	else if (res.size() ==2){
		res = '0'+res;
	}
	
	return res + ' ' + s;
	
	
}
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

string make_read(int fd){
	char size[l];
	read(fd,size,l);
	int len = atoi(size);
	//cout<<len<<endl;
	char *buffer = new char [len];
	read(fd,buffer,len);
	string str(buffer); 
	slice_string(str);
	return str;
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
	
	std::string return_to_server = "Node was inserted"; //result to be sent to client
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
	
	return "Adjacency was inserted";
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

			string aux = line.substr(0, node_id.size()); 
			if(aux == node_id){
				int tam = node_id.size(); 
				txt.push_back(line.substr(tam+3, line.size()-tam-3));
			}
			
			
			
		}
		myfile.close();
		
	}
	string all_adjacencies = "delete";
	
	for(int i=0;i<txt.size();++i){
		all_adjacencies += " " + txt[i];
	}
	
	
	
	return all_adjacencies ;
	
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
		cout<<txt[i]<<endl;
		outfile <<txt[i]<<endl;
	}
	
	outfile.close();
	
	return "Adjacency was deleted";
}
	
void delete_node(string node){
	
	std::size_t found = node.find('\0');
	
	node = node.substr(0,found);
	
	delSpaces(node);
	
	std::fstream myfile, outfile;
	
	//string slave_txt = "slave_" + to_string(num_file)+ ".txt";
	
	string slave_txt = "slave.txt";
	
	//cout<<slave_txt<<endl;
	myfile.open(slave_txt, ios::in);
	
	string line;
	
	vector<string> txt;
	bool begin = false;
	bool end = false;
	if (myfile.is_open()){
		while ( getline (myfile,line) && !myfile.eof()){
			//delSpaces(line);
			cout<<"Leyendo: |"<<line<<"|"<<endl;
			
			if(line == node){
				begin = true;
			}
			else if(begin==true){
				;
			}
			else if(begin == true && line == "" ){
				end = true;
			}
			else if(begin==true && end == true){
				txt.push_back(line);
			}
			else
			   txt.push_back(line);
			
			
		}
		myfile.close();
	}
	/*
	outfile.open(slave_txt, ios::out );
	outfile.clear();
	outfile.seekp(0);
	
	for(uint i=0;i<txt.size();++i){
		cout<<txt[i]<<endl;
		outfile <<txt[i]<<endl;
	}
	
	outfile.close();
	*/
	
}

void keepalive(){
	string respuesta="OK";
	write(SocketFD, respuesta.c_str(), respuesta.size());
}
	
void parse_message(string msg){
	string result;
	
	string type_query = slice_string(msg);
	transform(type_query.begin(), type_query.end(),type_query.begin(), ::tolower);
	
	
	if(type_query == "0"){
		cout<<"Inserting node"<<endl;
		result = insert_node(msg);
		result = size_string(result);
		write(SocketFD, result.c_str(), result.size());
	}
	else if(type_query == "1"){
		cout<<"Inserting adjacency"<<endl;
		result = insert_adjacency(msg);
		result = size_string(result);
		write(SocketFD, result.c_str(), result.size());
	}
	else if(type_query == "2"){
		cout<<"Select"<<endl;
		
		//res = select(msg);
	} 
	else if(type_query == "3"){
		cout<<"Delete node"<<endl;
		
		delete_node(msg);
	} 
	else if(type_query == "adj"){
		cout<<"Requesting all adjacencies of a node"<<endl;
		result = all_adjacencies(msg);
		result = size_string(result);
		write(SocketFD, result.c_str(), result.size());
	}
	else if(type_query == "4"){
		cout<<"Delete adjacency"<<endl;
		delete_adjacency(msg);
		
	} 
	else if(type_query == "5"){
		cout<<"Keep alive"<<endl;
		keepalive();
		
	} 
	else {
		result = "Error. Query not understood\n";
		result = size_string(result);
		write(SocketFD, result.c_str(), result.size());
	}
	
}

void requesting_access(int SocketFD, string identificador){ //new requesting access
	string request="Slave requesting access "+identificador;
	request = size_string(request);
	write(SocketFD, request.c_str(), request.size());
	
	sleep(1);
	identificador = size_string(identificador);
	write(SocketFD, identificador.c_str(), identificador.size());
	
	string response = make_read(SocketFD);
	
	if (response != "OK."){
		printf("Erroneous confirmation. Ending connection\n");
		shutdown(SocketFD, SHUT_RDWR);
		close(SocketFD);
	} else {
		string msg_ok = size_string("OK.");
		write(SocketFD, msg_ok.c_str(), msg_ok.size());
		printf("Connection to database as a slave established.\n");
	}
}
	
void send_msg(){
	int n;
	do{
		std::cout << "Type your message: ";
		string input ; 
		std::getline (std::cin,input);  	
		if (input[0] == '0'){
			end_connection = true;
		}
		
		input = size_string(input);
		write(SocketFD, input.c_str(), input.size()); //cuantos bytes estoy mandando
		
	} while(!end_connection);
}

	
		
void rcv_msg(){
	
	int n;
	do{	
		string temp = make_read(SocketFD);
		
		
	
		string result;
		if (n < 0) perror("ERROR reading from socket");
		
		if (temp.substr(0, 6) == "server"){
			
			slice_string(temp);
			cout<<"Mensaje de server"<<endl;
			cout<<temp<<endl;
			parse_message(temp);
			
			

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
	
