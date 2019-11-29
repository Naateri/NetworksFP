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
#include <queue>

#define MAX_SLAVES 2
#define uint unsigned int 

using namespace std;

const int l = 3;

int queryLevel = 1;


int SocketFD, ConnectFD;

string server_txt = "server.txt";

std::map<int, int> slaves; //Stored as: ID-FD
vector<bool> cur_ids; //vector to know available ids
vector<string> visitedNodes;

int node_counter = 0;   

int PORT = 40002;

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
	char *buffer = new char [len+1];
	int n = read(fd,buffer,len);
	//buffer[n] = '\n';
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
		int n2;
		
		
		string msg_slave = "server 2 " + msg + ' ' + to_string(lvl) + ' ' +to_string(CId);
		msg_slave = size_string(msg_slave);
		cout<<"Mensaje enviado al slave: " <<msg_slave<<endl;
		write(slaves[hash], msg_slave.c_str(), msg_slave.size());
		/*
		string tempNodeString (1,temp[0]);
		string cid = to_string(CId);
		string lengthString = to_string(cid.size());
		while(lengthString.size()<6){
			lengthString = "0"+lengthString;
		}
		msg_slave+=tempNodeString + to_string(lvl)+lengthString+cid;*/
		
		
		
		/*cout<<"Mensaje enviado al slave: " <<msg_slave<<endl;
		write(slaves[hash], msg_slave.c_str(), msg_slave.size());*/
		//n2 = read(slaves[hash],buffer2,256);
		//cout<<"Resultado"<<endl;
		//cout<<buffer2<<endl;
		//return_to_client = buffer2;
		return_to_client  =  "Making select query";
		/*if(buffer2[0] == 's'){
		
		n = read(slaves[hash],buffer2,6);
		string temp = buffer2;
		resTemp+="s"+temp;
		int resultSize = stoi(buffer2);
		n = read(slaves[hash],buffer2,resultSize);
		temp = buffer2;
		resTemp += temp;
		return_to_client = resTemp;
		}
		
		return return_to_client;*/
		//return "Node inserted";
		
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
	

string delete_node(string nodes){
	
	std::size_t found = nodes.find('\0');
	
	nodes = nodes.substr(0,found);
	
	
	vector<string> nodos = separate_string(nodes," ");
	for(int i=0;i<nodos.size();++i){
		delSpaces(nodos[i]);
	}
	
	string node_id = nodos[0];
	
	//cout<<"NODOS :"<<endl;
	bool connection_to_all = true; 
	for(int i=1;i<nodos.size();++i){
		//cout<<"|"<<nodos[i]<<"|"<<endl;
		connection_to_all &= verify_connection(nodos[i]);
		//cout<<"Conexion: "<<connection_to_all <<endl;
		if(connection_to_all == false){
			return  "Unconnected slave. Please try again later.";
		}
	}
	
	cout<<"Mandado nodo a borrar "<<endl;
	int ahash = hash_function(node_id);
	string node1 = "server 3 " + node_id;
	node1 = size_string(node1);
	write(ahash, node1.c_str(),node1.size());
	cout<<"---------"<<endl;
	
	
	string num_query = "server 4 ";
	string msg_slave1,msg_slave2; 
	int i;
	for(i=1;i<nodos.size();++i){
		int hash1 = hash_function(node_id);
		//cout<<"- "<<node_id << " - " << nodos[i]<< " deleted\n";
		msg_slave1 = "";
		msg_slave1 = num_query + node_id + " " + nodos[i];
		cout<<"+ "<<msg_slave1<<"|"<<msg_slave1.size()<<"|"<<endl;
		msg_slave1 = size_string(msg_slave1);
		write(slaves[hash1], msg_slave1.c_str(), msg_slave1.size());
		sleep(1);
	}
	
	for(i=1;i<nodos.size();++i){	
		msg_slave2 = "";
		int hash2 = hash_function(nodos[i]);
		string msg_slave2 = num_query + nodos[i] + " " + node_id;
		cout<<"+ "<<msg_slave2<<"|"<<msg_slave2.size()<<"|"<<endl;
		msg_slave2 = size_string(msg_slave2);
		write(slaves[hash2], msg_slave2.c_str(), msg_slave2.size());
		sleep(1);
		
	}	
	

	
	return "Node deleted";
	//cout<<"Node deleted"<<endl;
	
	
}

std::string delete_query(string msg){
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
		
		bool connection_slave = false;
	
		std::map<int,int>::iterator it;
		it = slaves.find(hash);
		if (it != slaves.end()){ 
			connection_slave = true;
			cout<<"Verifying the connection with the slave"<<endl;
			num_query = "server adj ";
			string msg_slave = num_query + msg ;
			msg = size_string(msg);
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
	
	int level;
	cout<< "Query : "<<str_msg<<endl;
	
	/// Saber cual es la consulta
	string query = slice_string(str_msg);
	transform(query.begin(), query.end(),query.begin(), ::tolower);
	//cout<<query<<endl;
	///
	
	//know what the server wants
	if (query == "insert" ){
		return insert(str_msg);
	} else if (query == "select"){
		visitedNodes.clear();
		
		string node_id = slice_string(str_msg);
		delSpaces(node_id);
		visitedNodes.push_back(node_id);
		//cout<<node_id<<endl;
		slice_string(str_msg);
		queryLevel = stoi(str_msg);
		//cout<<level<<endl;
		/*
		string tempLevelString(1,tempQuery[tempQuery.size()-1]);
		cout<<tempLevelString<<endl;
		level = std::stoi(tempLevelString);
		cout<<level<<endl;
		*/
		return select_node(node_id,1,CId);
		
		
	} 
	else if(query == "delete"){
		return delete_query(str_msg);
	}
	else {
		return "Error. Query not understood\n";
	}
}	

			
void rcv_msg(int ConnectFD, bool slave){

	bool end_connection = false;
	std::string slave_msg;
	std::string result;
	int n;
	do{
		sleep(1);
		string temp = make_read(ConnectFD);

		//if (n < 0) perror("ERROR reading from socket");
			
		if (slave){ //slave
			
			slice_string(temp);
			
			if(temp.substr(0,1)=="s"){
				cout<<"Temp: ";
				cout<< temp<<endl;
				slice_string(temp);
				string client_id = slice_string(temp);
				int id = stoi(client_id);
				queue<string> path;
				cout<<"ID: "<<id<<endl;
				cout<<"Query Level "<<queryLevel<<endl;
				vector<string>res = separate_string(temp,"/");
				vector<string> nodes;
				
				for(int i = 0; i<queryLevel;++i){
					cout<<"level "<<i<<endl;
					bool findNode = false;
					for(int j = 0;j<nodes.size();++j ){
						for(int k = 0 ; k< visitedNodes.size();++k){
							if(visitedNodes[k]==nodes[j]){
								findNode = true;
								break;
							}
						}
						if(!findNode){
							cout<<nodes[j]<<endl;
							cout<<"Nodo no visitado"<<endl;
							visitedNodes.push_back(nodes[j]);
							sleep(1);
							select_node(nodes[j],1,id);
						}
						
						findNode = false;
						
					}
					
					for(int j = 0; j<res.size();++j){
						delSpaces(res[j]);
						
						if(res[j]=="|"){
							string select_query = "sq " + temp;
							select_query = size_string(select_query);
							cout<<"QUERY SELECT: "<<select_query <<endl;
							write(id,select_query.c_str(),select_query.size());
							temp= "";
							cout<<"Encontro Fin"<<endl;
							break;
						}
						nodes = separate_string(res[j],"-");
						
					}
					
				}
				
				cout<<"termino"<<endl;
				/*string select_query = "sq " + temp;
				
				select_query = size_string(select_query);
				cout<<"QUERY SELECT: "<<select_query <<endl;
				/*int cidLength = stoi(temp.substr(1,6));
				int cidClient = stoi(temp.substr(7,cidLength));
				string message = "s"+temp.substr(7+cidLength,temp.size()-7+cidLength);
				cout<<"cidCLient"<<cidClient<<endl;*/
				//write(id,select_query.c_str(),select_query.size());
			
			
			}
			
		
			
			
			else if(temp.substr(0,6) == "delete"){ /// Se estan pasando las adyacencias del nodo que queremos borrar
				slice_string(temp);
				cout<<"Starting to delete node"<<endl;
				string algo = delete_node(temp); /// Borrar el nodo recien 
				algo = size_string(algo);
				write(ConnectFD, algo.c_str(),algo.size());
			}
			else
			cout<<"Slave : [ "<<temp <<" ]"<<endl; 
			
			
			//printf("Slave %d: [%s]\n", ConnectFD, buffer);
			/*
			char *mensaje = new char[men.size()+1];
			std::strcpy(mensaje, men.c_str());
			result = parse_message_slave(mensaje);
			*/
		}
		
		else if (temp == "Closing Connection."){
			end_connection = closing_connection(ConnectFD); //client ending connection
		}
		else if (!slave) {
			
			cout<<"You're a client.";
			result = parse_message_client(temp,ConnectFD);
			result = size_string(result);
			n = write(ConnectFD, result.c_str(), result.size());
			
			
			
		} else {
			cout<<"Query not understood"<<endl;
			std::string res = "Query not understood\n";
			
			res = size_string(res);
			
			n = write(ConnectFD, res.c_str(), res.size());
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
