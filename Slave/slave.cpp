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
		cout<<"No hay atributos"<<endl;
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
	


void insert_relation_slave(int num_file, string rel){
	std::fstream myfile, outfile;
	string slave_txt = "slave_" + to_string(num_file)+ ".txt";
	//cout<<slave_txt<<endl;
	myfile.open(slave_txt, ios::in);
	
	string line;
	
	vector<string> txt;
	txt.push_back(rel);
	//cout<<"INSERT_RELATION"<<endl;
	if (myfile.is_open()){
		while ( getline (myfile,line) && !myfile.eof()){
			txt.push_back(line);
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
	
}

int hash_function(std::string value){
	int cur_sum = 0;
	for (uint i = 0; i < value.size(); i++){
		cur_sum += (int)value[i];
	}
	return cur_sum % MAX_SLAVES;
}
	
string lrtrim(string str) {
	const std::string nothing = "" ;
	str = std::regex_replace( str, std::regex( "^\\s+" ), nothing ) ;
	str = std::regex_replace( str, std::regex( "\\s+$" ), nothing ) ;
	return str ; 
}

string insert_node(string s){
	std::string return_to_server; //result to be sent to client
	string node_id = slice_string(s);
	cout<<"NODE ID: " <<node_id<<endl;
	cout<<"RESTO: "<<s<<endl;
	
	
	vector<string> attr = separate_attributes(s);
	
	
	for(uint i=0;i<attr.size();++i){
		attr[i] = lrtrim(attr[i]);
		cout<<"|"<<attr[i]<<"|"<<endl;
	}
	
	for(uint i=0;i<attr.size();++i){
		cout<<"ATTRIBUTE:|"<<attr[i]<<"|"<<endl;
		cout<<"VALUE:|"<<attr[i+1]<<"|"<<endl;
		++i;
		
	}
	
	/// Cambiar por la que tiene que consultar al server 
	int hash = hash_function(node_id);
	/// -------
	
	/// Insertar en el slave
	
	string slave_txt = "slave_" + to_string(hash)+ ".txt";
	//cout<<slave_txt<<endl;
	ofstream fs;
	fs.open(slave_txt, ios::app);
	fs.seekp(0, ios::end);
	if(fs.is_open()){
		
		fs <<endl;
		fs<< node_id<<endl;
		for(uint i=0;i<attr.size();i+=2){
			cout<<"I: "<<i<<endl;
			fs<<attr[i]<<" : "<<attr[i+1]<<endl;
		}
	}

	fs.close();	
	
	return return_to_server;
	
}
	

	
string insert_relation(string s){
	vector<string> nodes = separate_string(s , " ");
	
	
	string rel1 =nodes[0] +" - "+nodes[1];
	string rel2 =nodes[1] +" - "+nodes[0];
	
	int hash1 = hash_function(nodes[0]);
	int hash2 = hash_function(nodes[1]);
	
	insert_relation_slave(hash1, rel1);
	insert_relation_slave(hash2, rel2);
	return "Relation inserted";
}
	
std::string parse_message(string msg){
	string res;
	
	string type_insert = slice_string(msg);
	transform(type_insert.begin(), type_insert.end(),type_insert.begin(), ::tolower);
	
	
	if(type_insert == "0"){
		cout<<"Insertando un nodo"<<endl;
		res = insert_node(msg);
	}
	else if(type_insert == "1"){
		cout<<"Insertando una relacion"<<endl;
		res = insert_relation(msg);
	} else {
		res = "Error. Query not understood\n";
	}
	
	return res;
	
}


void requesting_access(int SocketFD){
	write(SocketFD, "Requesting access.", 19);
	bzero(buffer,256);
	 read(SocketFD,buffer,255);
	
	if (strcmp(buffer, "OK.") != 0){
		printf("Erroneous confirmation. Ending connection\n");
		shutdown(SocketFD, SHUT_RDWR);
		close(SocketFD);
	} else {
		 write(SocketFD, "OK.", 3);
		printf("Connection to database established\n");
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
	//	cout<<"TEMP:"<<temp<<"|"<<endl;
		std::size_t found = temp.find('\0');
		//cout<<"POS"<<found<<endl;
		temp = temp.substr(0,found);
	//	cout<<"TEMP:"<<temp<<"|"<<endl;
		string result;
		if (n < 0) perror("ERROR reading from socket");

		
		if (temp.substr(0, 6) == "server"){
			
			slice_string(temp);
			cout<<temp<<endl;
			result = parse_message(temp);
			/*n = write(SocketFD, result.c_str(), result.size());
			*/
			
		}
		
		
		
		
		
		
		
		printf("Server: [%s]\n",buffer);
	} while(strcmp(buffer, "chau") != 0);
	
	end_connection = false;
	
}
			
int main(void){
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
	
	requesting_access(SocketFD);
	
	std::thread t1(send_msg);
	std::thread t2(rcv_msg);
	
	t1.join();
	t2.join();
	shutdown(SocketFD, SHUT_RDWR);
	
	close(SocketFD);
	
	return 0;
}
	
