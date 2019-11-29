/* Client code in C++ */

//g++ -std=c++0x -o slaveb.exe slaveb.cpp

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

int SocketFD1;
char buffer[256];

std::string IP = "127.0.0.1";
int PORT = 40005;
int PORTSLAVE = 50007;

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
	

	
int hash_function(std::string value){
	int cur_sum = 0;
	for (uint i = 0; i < value.size(); i++){
		cur_sum += (int)value[i];
	}
	return cur_sum % MAX_SLAVES + 1;
}
	

	


void keepalive(){
	string respuesta="008 1024";
	write(SocketFD, respuesta.c_str(), respuesta.size());
	cout<<"envía"<<endl;
}
	
void parse_message(string msg){
	string result;
	
	string type_query = slice_string(msg);
	transform(type_query.begin(), type_query.end(),type_query.begin(), ::tolower);
	
	
	
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
void requesting_access_backup(int SocketFD1, string identificador){ //new requesting access
	string request="SlaveBackup requesting access "+identificador;
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
		
	}
	printf("Connection to database as a slavebackup established.\n");
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
		
		//cout<<temp<<endl;
	
		string result;
		if (n < 0) perror("ERROR reading from socket");
		
		string aux = buffer;
		vector<string> select = separate_string(aux, "/");
		
		ofstream fs("backup.txt");
		//string slave_txt = "backup.txt";	
		
		
		
		for(int i=0;i<select.size();++i){
			fs<<select[i]<<endl;
		}
			
		fs.close();
		

		//printf("Server: [%s]\n",buffer);
	} while(!end_connection);
	
	end_connection = true;
}
			
int main(void){
	cout << "SLAVE"<<endl;
	
	
	//----------------------------
	//----------------------------
	//Conexion con el servidor mandando el mismo identificador que el slave del que es backup
	
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
	
	string id = to_string(get_id());
	requesting_access_backup(SocketFD,id);
	
	//std::thread t1(send_msg);
	std::thread t2(rcv_msg);
	
	//t1.join();
	t2.join();
		
	

	shutdown(SocketFD, SHUT_RDWR);
	
	close(SocketFD);
	
	return 0;
}
	