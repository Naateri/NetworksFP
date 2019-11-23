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
#define uint unsigned int 
using namespace std;
int SocketFD, ConnectFD;
char msg[256];
string server_txt = "server.txt";

std::map<int, int> slaves; //Stored as: ID-FD
int node_counter = 0;   

int PORT = 40000;

///Para probar 
string men = "INSERT A - C"; 
///

string slice_string(string &s){
	string delimiter = " ";
	int pos = s.find(delimiter);
	string s1 = s.substr(0, pos);
	s.erase(0, pos + delimiter.length());
	return s1;
}
	
int hash_function(std::string value){
	int cur_sum = 0;
	for (uint i = 0; i < value.size(); i++){
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

bool closing_connection(int ConnectFD){
	char msg[5];
	int n = write(ConnectFD, "OK.", 4);
	printf("OK. sent to %d\n", ConnectFD);
	sleep(1); //giving time for client to process reply
	n = write(ConnectFD, "Are you sure?", 13+1);
	n = read(ConnectFD, msg, 5); //yes
	if (strcmp(msg, "Yes.") == 0){
		printf("Ending connection with client %d\n", ConnectFD);
		return true;
	} else return false;
}

void delSpaces(string &s){
	s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
}

void insert_node(string &msg){
	vector<string> node = separate_string(msg, "{");
	delSpaces(node[0]);
	string node_id = node[0];
	
	//cout<<"Nombre del nodo: |"<< node_id <<"|\n";
	
	int hash = hash_function(node_id);
	/// Insertar en server.txt
	/*ofstream fe;
	fe.open(server_txt, ios::app);
	fe.seekp(0, ios::end);
	string node_to_insert =  node_id + ' ' + to_string(hash) + '\n';
	fe << node_to_insert;
	fe.close();	*/
	
	
	/// Madarle al slave lo que tiene que insertar
	/// 0 es para insertar un nodo 
	/// 1 es para insertar una relacion
	
	string msg_slave = "server 0 " + msg;
	write(ConnectFD, msg_slave.c_str(), msg_slave.size());
}
	

	
std::string insert(string msg){
	std::string return_to_client; //result to be sent to client
	//cout<<msg<<endl;
	bool isNode= false;
	for(uint i=0;i<msg.size();++i){
		if(msg[i] == '{'){
			isNode = true;
			break;
		}
		else if(msg[i] == '-'){
			break;
		}
		
	}

	/// Insertando nodo
	if(isNode == true){
		
		/// Conexion con el slave	
		bool connection_slave = true ;
		///

		if (connection_slave ){
			insert_node(msg);
			return_to_client = "Node inserted";
		}
		else
			return_to_client = "Unconnected slave. Please try again later.";
		
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
		
		/// Verificar conexion con ambos slaves
		bool connection_slave1 = true;
		bool connection_slave2 = true;
		
		
		if(connection_slave1 && connection_slave2){
			/// Insertar en server.txt
			ofstream fe;
			fe.open(server_txt, ios::app);
			fe.seekp(0, ios::end);
			fe << nodes[0] + " " +to_string(hash1)<<endl;
			fe << nodes[1] + " " +to_string(hash2)<<endl;
			fe.close();	
			
			string msg_slave = "server 1 " + nodes[0] + " " + nodes[1];
			 write(ConnectFD, msg_slave.c_str(), msg_slave.size());
			return_to_client = "Relation inserted";
		}
		else
		   return_to_client = "Unconnected slave. Please try again later.";
	}
	
	//std::string temp(msg); //communicate with appropiate slave

	//put in temp the node value
	//int hash = hash_function(temp);
	
	//comunicate with appropiate slave
	
	return return_to_client;
}

std::string parse_message(char* msg){
	//int n = 0, level; //n = message type
	string str_msg(msg, strlen(msg));
	
	
	/// Saber cual es la consulta
	string query = slice_string(str_msg);
	transform(query.begin(), query.end(),query.begin(), ::tolower);
	//cout<<query<<endl;
	///
	
	//know what the server wants
	if (query == "insert" ){
		return insert(str_msg);
	}  else {
		return "Error. Query not understood\n";
	}
}
			
void rcv_msg(int ConnectFD){
	char buffer[256];
	bool end_connection = false;
	std::string slave_msg;
	//bool msg_from_slave = false;
	std::string result;
	int n;
	do{
		bzero(buffer,256);
		n = read(ConnectFD,buffer,255);
		std::string temp(buffer, 256);
		if (n < 0) perror("ERROR reading from socket");

		if (temp.substr(0, 5) == "slave"){
			printf("Slave %d: [%s]\n", ConnectFD, buffer);
			
			char *mensaje = new char[men.size()+1];
			std::strcpy(mensaje, men.c_str());
			parse_message(mensaje);
		} else if (strcmp(buffer, "Closing Connection.") == 0){
			end_connection = closing_connection(ConnectFD); //client ending connection
		}
		else{
			cout<<"Query not understood"<<endl;
			std::string res = "Query not understood\n";
			n = write(ConnectFD, res.c_str(), res.size());
		}
	/*	if (!msg_from_slave){
			result = parse_message(buffer);
			n = write(ConnectFD, result.c_str(), result.size());
		}*/
		
	}while (!end_connection); //( strcmp(msg, "chau") != 0);
	
	shutdown(ConnectFD, SHUT_RDWR);
	
	close(ConnectFD);
}
				
int main(void) {
	struct sockaddr_in stSockAddr;
	SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	//int Res;
	
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
