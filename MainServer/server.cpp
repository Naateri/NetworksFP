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

#define MAX_SLAVES 1
#define uint unsigned int 

using namespace std;

const int l = 3;


int SocketFD, ConnectFD;

string server_txt = "server.txt";

std::map<int, int> slaves; //Stored as: ID-FD
vector<bool> cur_ids; //vector to know available ids

int node_counter = 0;   

int PORT = 40008;
///Para probar 
string men = "INSERT Hola {loquesea:val, otro:bai}"; 
///

string lrtrim(string str) {
	const std::string nothing = "" ;
	str = std::regex_replace( str, std::regex( "^\\s+" ), nothing ) ;
	str = std::regex_replace( str, std::regex( "\\s+$" ), nothing ) ;
	return str ; 
}

string size_string(string s){
	int num = s.size();
	num += 1; // " "
	string res = to_string(num);
	
	if(res.size() == 1)
		res = "00"+res;	 
	else if (res.size() ==2){
		res = '0'+res;
	}
	
	return res + ' ' + s;
}
string slice_string(string &s){
	string delimiter = " ";
	int pos = s.find(delimiter);
	string s1 = s.substr(0, pos);
	s.erase(0, pos + delimiter.length());
	return s1;
}
	
string make_read(int fd){
	//cout<<"Make read"<<endl;
	char size[l];
	read(fd,size,l);
	int len = atoi(size);
	//cout<<"TAM"<<len<<endl;
	char *buffer = new char [len];
	read(fd,buffer,len);
	string str(buffer); 
	
	str = lrtrim(str);
	str.resize(len-1);
	//cout<<"STRING: |"<<str<<"|"<<endl;
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
	
void delSpaces(string &s){
	s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
}	
	
	
int hash_function(std::string value){
	int cur_sum = 0;
	for (uint i = 0; i < value.size(); i++){
		cur_sum += (int)value[i];
	}
	return cur_sum % MAX_SLAVES + 1;
}

int confirming_connection(int ConnectFD, string identificador = ""){ //0 -> client, 1 -> slave, -1 -> error
	
	string str = make_read(ConnectFD);
	//cout<<"DEl cliente |"<<str<<"|"<<endl;
	
	if (str == "Requesting access."){ //client
		
		string confirm = size_string("OK.");
		//cout<<"Mandando"<<confirm<<endl;
		write(ConnectFD, confirm.c_str(), confirm.size());
		
		string str1 = make_read(ConnectFD); 
		//cout<<"STR1 "<<str1<<endl;
		return 0;
		
	} 
	
	else if(str.substr(0, 23) =="Slave requesting access"){ //slave
		
		//slice_string(str);
		str = str.substr(23,str.size()-23);
		//cout<<"ID|"<<str<<"|"<<endl;
		
		int identificador = stoi(str);
		
		if (identificador == 0){ //server gives id to slave
			for(uint i = 0; i < cur_ids.size(); i++){
				if (cur_ids[i] == false){
					identificador = i+1;
				}
			}
		}
		
		std::map<int,int>::iterator it;
		it = slaves.find(identificador);
		if (it != slaves.end()){ //id found
			
			string notfound = size_string("No.");
			write(ConnectFD, notfound .c_str(), notfound .size());
			return -1;
		}
		else{
			slaves[identificador] = ConnectFD;
			
			cur_ids[identificador-1] = true;
			
			cout<<"Slave registered, identifier: " << identificador <<endl;
			
			string confirm = size_string("OK.");
			write(ConnectFD, confirm.c_str(), confirm.size());
			
			//make_read(ConnectFD);
			
			return 1;
		}
	} 
	else {
		string notfound = size_string("No.");
		write(ConnectFD, notfound .c_str(), notfound .size());
		return -1;
	}
}


bool closing_connection(int ConnectFD){ //client
	string confirm = size_string("OK.");
	write(ConnectFD, confirm.c_str(), confirm.size());
	
	printf("OK. sent to %d\n", ConnectFD);
	sleep(1); //giving time for client to process reply
	
	string confirm2 = size_string("Are you sure?");
	write(ConnectFD, confirm2.c_str(), confirm2.size());

	string str = make_read(ConnectFD);
	
	
	
	if (str == "Yes."){
		printf("Ending connection with client %d\n", ConnectFD);
		return true;
	} else return false;
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

		bool connection_slave = false ;
		vector<string> node = separate_string(msg, "{");
		delSpaces(node[0]);
		string node_id = node[0];
		int hash = hash_function(node_id);
		
		std::map<int,int>::iterator it;
		it = slaves.find(hash);
		if (it != slaves.end()){ 
			connection_slave = true;
		}

		if (connection_slave){
			

		
			/// Madarle al slave lo que tiene que insertar
			/// 0 es para insertar un nodo 
			/// 1 es para insertar una relacion
			
			string msg_slave = "server 0 " + msg;
			
			msg_slave = size_string(msg_slave);
			
			write(slaves[hash], msg_slave.c_str(), msg_slave.size());
			
			return "Node inserted";
			
		}
		else
			return "Unconnected slave. Please try again later.";
		
	}
	
	/// Insertando relacion
	else{ 
		
		vector<string> nodes = separate_string(msg,"-");
		
		delSpaces(nodes[0]);
		delSpaces(nodes[1]);
		
		//cout<<"Nodo 0: "<<nodes[0] <<endl;
		//cout<<"Nodo 1: "<<nodes[1] <<endl;
		
		int hash1 = hash_function(nodes[0]);
		int hash2 = hash_function(nodes[1]);
		
		/// Verificar conexion con ambos slaves
		bool connection_slave1 = false;
		bool connection_slave2 = false;
		
		std::map<int,int>::iterator it;
		it = slaves.find(hash1);
		if (it != slaves.end()){ 
			connection_slave1 = true;
		}
		it = slaves.find(hash2);
		if (it != slaves.end()){ 
			connection_slave2 = true;
		}
		
		//cout<< "Slave 1: "<<connection_slave1<<" Hash : "<<hash1<< endl;
		//cout<< "Slave 2: "<<connection_slave2<< "Hash : "<<hash2<<endl;
		
		if(connection_slave1 && connection_slave2){
			if(connection_slave1){
				cout<<"- "<<nodes[0] << " - " << nodes[1]<< " inserted\n";
				string msg_slave = "server 1 " + nodes[0] + " " + nodes[1];
				msg_slave = size_string(msg_slave);
				write(slaves[hash1], msg_slave.c_str(), msg_slave.size());
			}
			
			if( connection_slave2){
				cout<<"- "<<nodes[1] << " - " << nodes[0]<< " inserted\n";
				string msg_slave = "server 1 " + nodes[1] + " " + nodes[0];
				msg_slave = size_string(msg_slave);
				write(slaves[hash2], msg_slave.c_str(), msg_slave.size());
			}
			return "Adjacency inserted";
		}
		
		else
		   return "Unconnected slave. Please try again later.";
		
	}


}
	
std::string select_node(string msg, int lvl,int CId){
	
	
	std::string return_to_client; //result to be sent to client
	//put in temp the node value
	int hash = hash_function(msg);
	cout<<"Client ID: "<<CId<<endl;
	
	bool connection_slave = false ;
	std::map<int,int>::iterator it;
	it = slaves.find(hash);
	if (it != slaves.end()){
		
		connection_slave = true;
	}

	
	/// Cambiar
	if (connection_slave){
		cout <<"Encontro slave"<<endl;
		
		char buffer2[2048];
		bzero(buffer2,2048);
		string resTemp;
		
		
		string msg_slave = "server 2 " + msg + ' ' + to_string(lvl) + ' ' +to_string(CId);
		msg_slave = size_string(msg_slave);
		cout<<"Mensaje enviado al slave: " <<msg_slave<<endl;
		write(slaves[hash], msg_slave.c_str(), msg_slave.size());
	
		return_to_client  =  "Making select query";
		
	}else{
		return "Unconnected slave. Please try again later.";
	}
	
	return return_to_client;
}
	
bool verify_connection(string n){
	bool connection = false;
	int hash = hash_function(n);
	std::map<int,int>::iterator it;
	it = slaves.find(hash);
	if (it != slaves.end()){ 
		connection = true;
	}
	
	return connection;
	
}
	

void delete_node(string nodes){
	vector<string> nodos = separate_string(nodes," ");
	for(uint i=0;i<nodos.size();++i){
		delSpaces(nodos[i]);
		//cout<<nodos[i]<<endl;
	}
	
	string node_id = nodos[0];
	string fd = nodos[nodos.size()-1];
	//cout<<"FD"<<fd<<endl;
	
	//cout<<"NODOS :"<<endl;
	bool connection_to_all = true; 
	for(uint i=1;i<nodos.size()-1;++i){
		//cout<<"|"<<nodos[i]<<"|"<<endl;
		connection_to_all &= verify_connection(nodos[i]);
		//cout<<"Conexion: "<<connection_to_all <<endl;
		if(connection_to_all == false){
			string response = "Unconnected slave. Please try again later.";
			size_string(response);
			write(stoi(fd), response.c_str(), response.size());
			return;  
		}
	}
	
	//cout<<"Mandado nodo a borrar "<<endl;
	int ahash = hash_function(node_id);
	string node1 = "server 3 " + node_id ;
	node1 = size_string(node1);
	write(slaves[ahash], node1.c_str(),node1.size());
	//cout<<"---------"<<endl;
	
	
	string num_query = "server 4 ";
	string msg_slave1,msg_slave2; 
	
	for(uint i=1;i<nodos.size()-1;++i){
		int hash1 = hash_function(node_id);
		//cout<<"- "<<node_id << " - " << nodos[i]<< " deleted\n";
		msg_slave1 = "";
		msg_slave1 = num_query + node_id + " " + nodos[i];
		//cout<<"+ "<<msg_slave1<<"|"<<msg_slave1.size()<<"|"<<endl;
		msg_slave1 = size_string(msg_slave1);
		write(slaves[hash1], msg_slave1.c_str(), msg_slave1.size());
		//sleep(1);
	}
	
	for(uint i=1;i<nodos.size()-1;++i){	
		msg_slave2 = "";
		int hash2 = hash_function(nodos[i]);
		string msg_slave2 = num_query + nodos[i] + " " + node_id;
		//cout<<"+ "<<msg_slave2<<"|"<<msg_slave2.size()<<"|"<<endl;
		msg_slave2 = size_string(msg_slave2);
		write(slaves[hash2], msg_slave2.c_str(), msg_slave2.size());
		//sleep(1);
		
	}	
	
	string response = "Node " + node_id +" deleted";
	response = size_string(response);
	write(stoi(fd), response.c_str(), response.size());
	cout<<response<<endl;
	
}

std::string delete_query(string msg, int fd){
	std::string return_to_client; //result to be sent to client
	string num_query; 
	//cout<<msg<<endl;
	bool deleteNode = true;
	for(uint i=0;i<msg.size();++i){
		if(msg[i] == '-'){
			deleteNode = false;
			break;
		}
	}
	
	/// Borrando nodo
	if(deleteNode == true){
		delSpaces(msg);
		int hash = hash_function(msg);
		
		//bool connection_slave = false;
	
		std::map<int,int>::iterator it;
		it = slaves.find(hash);
		if (it != slaves.end()){ 
			//connection_slave = true;
			cout<<"Verifying the connection with the slave"<<endl;
			num_query = "server adj ";
			string msg_slave = num_query + msg +' ' +to_string(fd) ;
			msg_slave = size_string(msg_slave);
			write(slaves[hash], msg_slave.c_str(), msg_slave.size());
			
			return "Verifying the connection with the slave.";
		}
		else
		
			return "Unconnected slave. Please try again later.";
	}
	
	/// Borrando relacion
	else{ 
		vector<string> nodes = separate_string(msg,"-");
		
		delSpaces(nodes[0]);
		delSpaces(nodes[1]);
		
		int hash1 = hash_function(nodes[0]);
		int hash2 = hash_function(nodes[1]);
		
		/// Verificar conexion con ambos slaves
		bool connection_slave1 = false;
		bool connection_slave2 = false;
		
		std::map<int,int>::iterator it;
		it = slaves.find(hash1);
		if (it != slaves.end()){ 
			connection_slave1 = true;
		}
		it = slaves.find(hash2);
		if (it != slaves.end()){ 
			connection_slave2 = true;
		}
		
		if(connection_slave1 && connection_slave2){
			num_query = "server 4 ";
			if(connection_slave1){
				cout<<"- "<<nodes[0] << " - " << nodes[1]<< " deleted\n";
				string msg_slave = num_query + nodes[0] + " " + nodes[1];
				msg_slave = size_string(msg_slave);
				write(slaves[hash1], msg_slave.c_str(), msg_slave.size());
			}
			
			if( connection_slave2){
				cout<<"- "<<nodes[1] << " - " << nodes[0]<< " deleted\n";
				string msg_slave = num_query + nodes[1] + " " + nodes[0];
				msg_slave = size_string(msg_slave);
				write(slaves[hash2], msg_slave.c_str(), msg_slave.size());
			}
			return "Adjacency deleted";
		}
		
		else
		   return "Unconnected slave. Please try again later.";
	
	}
}
	

std::string parse_message_client(string str_msg,int CId){
	

	/// Saber cual es la consulta
	string query = slice_string(str_msg);
	transform(query.begin(), query.end(),query.begin(), ::tolower);

	//know what the server wants
	if (query == "insert" ){
		return insert(str_msg);
	} else if (query == "select"){
		string node_id = slice_string(str_msg);
		delSpaces(node_id);
		slice_string(str_msg);
		int level = stoi(str_msg);
		return select_node(node_id,level,CId);
		
		
	} 
	else if(query == "delete"){
		//cout<<"STR_MSG EN DELETE"<< str_msg<<endl;
		return delete_query(str_msg, CId);
	}
	else {
		return "Error. Query not understood\n";
	}
}	

			
void rcv_msg(int ConnectFD, bool slave){

	bool end_connection = false;
	std::string slave_msg;
	std::string result;
	do{
		sleep(1);
		string temp = make_read(ConnectFD);

		if (slave){ //slave
			
			slice_string(temp);
			
			if(temp.substr(0,1)=="s"){
				slice_string(temp);
				string client_id = slice_string(temp);
				int id = stoi(client_id);
				string select_query = "sq " + temp;
				select_query = size_string(select_query);
				write(id,select_query.c_str(),select_query.size());
			}
			
			else if(temp.substr(0,6) == "delete"){ /// Se estan pasando las adyacencias del nodo que queremos borrar
				//cout<<"LLEGO: "<<temp<<endl;
				slice_string(temp);
				cout<<"Starting to delete node"<<endl;
				delete_node(temp); /// Borrar el nodo recien 
				//algo = size_string(algo);
				/*write(ConnectFD, algo.c_str(),algo.size());*/
			}
			else
			cout<<"Slave : [ "<<temp <<" ]"<<endl; 
		}
		
		else if (temp == "Closing Connection."){
			end_connection = closing_connection(ConnectFD); //client ending connection
		}
		else if (!slave) {
			cout<<"You're a client."<<endl;
			result = parse_message_client(temp,ConnectFD);
			result = size_string(result);
			write(ConnectFD, result.c_str(), result.size());
		} 
		else {
			cout<<"Query not understood"<<endl;
			std::string res = "Query not understood\n";
			res = size_string(res);
			write(ConnectFD, res.c_str(), res.size());
		}
		
	}while (!end_connection);
	
	shutdown(ConnectFD, SHUT_RDWR);
	
	close(ConnectFD);
}
				
int main(void) {
	cout<<"SERVER"<<endl;
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
