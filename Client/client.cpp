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

int SocketFD;
char buffer[256];

std::string IP = "127.0.0.1";
int PORT = 40000;

bool end_connection = false;

void requesting_access(int SocketFD){
	int n = write(SocketFD, "Requesting access.", 18);
	bzero(buffer,256);
	n = read(SocketFD,buffer,255);
	
	if (strcmp(buffer, "OK.") != 0){
		printf("Erroneous confirmation. Ending connection\n");
		shutdown(SocketFD, SHUT_RDWR);
		close(SocketFD);
	} else {
		n = write(SocketFD, "OK.", 3);
		printf("Connection to database established\n");
	}
}

void send_msg(){
	char msg[256];
    int n;
	do{
		std::cout << "Type your message: ";
		std::cin.getline(msg, 255);
		n = write(SocketFD,msg,255); //cuantos bytes estoy mandando

		//n dice cuantos bytes se han mandado
		msg[n] = '\0';
	} while(!end_connection);
}

void rcv_msg(){
	//char buffer[256];
    int n;
	do{	
		bzero(buffer,256);
		n = read(SocketFD,buffer,255);
		if (n < 0) perror("ERROR reading from socket");
		buffer[n] = '\0';
		printf("Server: [%s]\n",buffer);
	} while(strcmp(buffer, "chau") != 0);
	
	end_connection = false;
	
}
 
int main(void){
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
