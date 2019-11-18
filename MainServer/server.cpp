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

#define MAX_SLAVES 5

int SocketFD, ConnectFD;
char msg[256];

int PORT = 40000;

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

std::string insert_node(const char* msg, int lvl){
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
	
	//comunicate with appropiate slave
	
	return return_to_client;
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
