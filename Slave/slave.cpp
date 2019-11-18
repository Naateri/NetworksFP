  /* Slave code in C++ */

//g++ slave.cpp -o slave -std=c++11 -lpthread
 
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

#define MAX_SLAVES 5

//Message from Slave to server starts with SLAVE_MSG

int SocketFD, ConnectFD;
char msg[256];

int PORT = 40000;
std::string IP = "127.0.0.1";

int ask_server_for_id(int SocketFD){
	int id;
	//ask server
	//current IDs
	//return first one available
	return id;
}

int check_id(int SocketFD){
	int id;
	std::ifstream text_file("database.txt");
	if (text_file.is_open()){
		//read first value from txt
		//return id
		return id;
	} else { 
		//file doesn't exist
		//ask server
		return ask_server_for_id(SocketFD);
	}
}

std::string insert_node(const char* msg, int lvl){
	std::string temp(msg); //communicate with appropiate slave
	std::string return_to_server; //result to be sent to client
	
	//insert node in txt
	
	std::ofstream text_file("database.txt");
	
	return return_to_server;
}


std::string select_node(const char* msg, int lvl){
	std::string temp(msg); //communicate with appropiate slave
	std::string return_to_server; //result to be sent to client
	
	//select correct connections
	
	std::ifstream text_file("database.txt");
	
	return return_to_server;
}

std::string parse_message(char msg[256]){
	int n = 0, level; //n = message type
	std::string str_msg(msg, strlen(msg));
	//know what the server wants
	if (n == 1){
		return insert_node(str_msg.c_str(), level);
	} else if (n == 2){
		return select_node(str_msg.c_str(), level);
	} else {
		return "Error. Query not understood\n";
	}
}

void rcv_msg(int SocketFD){
	char buffer[256];
    int n;
	std::string result;
	do{
		bzero(buffer,256);
		n = read(SocketFD,buffer,255);
		if (n < 0) perror("ERROR reading from socket");
		printf("Client %d: [%s]\n", SocketFD, buffer);
		
		result = parse_message(buffer);
		
	}while ( strcmp(msg, "chau") != 0);
	
	shutdown(SocketFD, SHUT_RDWR);
	
	close(SocketFD);
}

int main(void) {
	struct sockaddr_in stSockAddr;
	int Res;
	SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int n;
	char msg[256];
	
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
	if (-1 == cnct)
	{
		perror("connect failed");
		close(SocketFD);
		exit(EXIT_FAILURE);
	}
	
	//checking ID
	//check_id(SocketFD);
	
	std::thread t1(rcv_msg, SocketFD);
	
	t1.join();
	shutdown(SocketFD, SHUT_RDWR);
	
	close(SocketFD);
    return 0;
}
