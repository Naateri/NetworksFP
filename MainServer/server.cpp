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
vector<bool> cur_ids; //vector to know available ids

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

int confirming_connection(int ConnectFD, string identificador = ""){ //0 -> client, 1 -> slave, -1 -> error
	char buffer[256];
	int n = read(ConnectFD, buffer, 26);
	if (strcmp("Requesting access.", buffer) == 0){ //client
		n = write(ConnectFD, "OK.", 4);
		bzero(buffer, 256);
		n = read(ConnectFD, buffer, 4);
		return 0;
	} else if(strcmp("Slave requesting access", buffer) == 0){ //slave
		
		bzero(buffer, 256);
		
		n= read(ConnectFD,buffer,3); //identifier
		buffer[n]='\0';
		
		int identificador = atoi(buffer);
		
		if (identificador == 0){ //server gives id to slave
			for(int i = 0; i < cur_ids.size(); i++){
				if (cur_ids[i] == false){
					identificador = i+1;
				}
			}
		}
		
		std::map<int,int>::iterator it;
		it = slaves.find(identificador);
		if (it != slaves.end()){ //id found
			n = write(ConnectFD, "No.", 4);
			return -1;
		}
		else{
			slaves[identificador]=ConnectFD;
			
			cur_ids[identificador-1] = true;
			
			cout<<"Slave registered, identifier: " << identificador <<endl;
			n = write(ConnectFD, "OK.", 4);
			
			n = read(ConnectFD, buffer, 4);
			return 1;
		}
	} else {
		n = write(ConnectFD, "No.", 3);
		return -1;
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

bool closing_connection(int ConnectFD){ //client
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

		if (connection_slave){
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
	vector<string> nodes;
	nodes.push_back(separate[0]);
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
						vector<string> temp = separate_string(line, " ");
						nodes.push_back(temp[temp.size()-1]);
					}
				}else{
					for(int i = 0;i<nodes.size();++i){
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
			return_to_client = "s"+lengthString+res;
			file.close();
		}
		
	}	
	
	//comunicate with appropiate slave
	
	return return_to_client;
}

std::string parse_message(char* msg){
	
	int level;
	string str_msg(msg, strlen(msg));
	
	/// Saber cual es la consulta
	string query = slice_string(str_msg);
	transform(query.begin(), query.end(),query.begin(), ::tolower);
	//cout<<query<<endl;
	///
	
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
			
void rcv_msg(int ConnectFD, bool slave){
	char buffer[256];
	bool end_connection = false;
	std::string slave_msg;
	std::string result;
	int n;
	do{
		bzero(buffer,256);
		n = read(ConnectFD,buffer,255);
		std::string temp(buffer, 256);
		if (n < 0) perror("ERROR reading from socket");

		if (temp.substr(0, 5) == "slave" && slave){ //slave
			printf("Slave %d: [%s]\n", ConnectFD, buffer);
			
			char *mensaje = new char[men.size()+1];
			std::strcpy(mensaje, men.c_str());
			result = parse_message(mensaje);
		} else if (strcmp(buffer, "Closing Connection.") == 0){
			end_connection = closing_connection(ConnectFD); //client ending connection
		}else if (!slave) {
			std::string test = "You're a client.";
			n = write(ConnectFD, test.c_str(), test.size());
		} else {
			cout<<"Query not understood"<<endl;
			std::string res = "Query not understood\n";
			n = write(ConnectFD, res.c_str(), res.size());
		}
		
	}while (!end_connection);
	
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
	
	for (int i = 0; i < MAX_SLAVES; i++){ //setting all slave ids to false
		cur_ids.push_back(false);
	}

	for(;;){
		ConnectFD = accept(SocketFD, NULL, NULL);
		
		if(0 > ConnectFD){
			perror("error accept failed");
			close(SocketFD);
			exit(EXIT_FAILURE);
		}
		
		int connection = confirming_connection(ConnectFD);
		if (connection == 0){
			/* perform read write operations ... */
			std::thread t1(rcv_msg, ConnectFD, false); //non-slave
			t1.detach();
		} else if (connection == 1) { //slave
			std::thread t1(rcv_msg, ConnectFD, true); //slave
			t1.detach();
		} else { //error
			shutdown(ConnectFD, SHUT_RDWR);
			close(ConnectFD);
		}
	}
	
	close(SocketFD);
	return 0;
}
