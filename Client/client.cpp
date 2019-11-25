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
using namespace std;

int SocketFD;
char buffer[256];

std::string IP = "127.0.0.1";
int PORT = 40003;

bool end_connection = false;

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

void closing_connection(){
	
	write(SocketFD, "Closing Connection.", 19+1);
	bzero(buffer,256);
	read(SocketFD,buffer,4); //OK
	
	if (strcmp(buffer, "OK.") == 0){
		bzero(buffer,256);
		read(SocketFD, buffer, 13+2); //Are you sure?
		if (strcmp(buffer, "Are you sure?") == 0){
			write(SocketFD, "Yes.", 4+1);
			printf("Ending connection with server. Closing socket.\n");
		} else {
			printf("Erroneous confirmation. Server not asking.\n");
		}
	} else {
		printf("Erroneous confirmation. Server not confirming.\n");
	}
	
}

void send_msg(){
	char msg[256];
    int n;
	do{
		std::cout << "Type your message: ";
		string input ; 
		std::getline (std::cin,input);
		input = "client " + input;
		
		strcpy(msg, input.c_str()); 
		
		
		n = write(SocketFD,msg,255); //cuantos bytes estoy mandando

		//n dice cuantos bytes se han mandado
		msg[n] = '\0';
		
		if ((char)msg[0] == '0'){
			end_connection = true;
		}
		
	} while(!end_connection);
}

void rcv_msg(){
	char buffer[2048];
	//string buffer;
    int n;
	do{	
		bzero(buffer,2048);
		/*n = read(SocketFD,buffer,1);
		if(buffer[0] == 's'){
			n = read(SocketFD,buffer,6);
			int resultSize = stoi(buffer);
			n = read(SocketFD,buffer,resultSize);
		}*/
		
		n = read(SocketFD, buffer, 2048);
		
		if (n < 0) perror("ERROR reading from socket");
		buffer[n] = '\0';
		printf("Server: [%s]\n",buffer);
	} while (!end_connection);//while(strcmp(buffer, "chau") != 0);
	
	end_connection = true;
	
}
 
int main(void){
	cout<<"CLIENT"<<endl;
    struct sockaddr_in stSockAddr;
    int Res;
    SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   
   // char msg[256];
 
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
	
	closing_connection();
	
    shutdown(SocketFD, SHUT_RDWR);
 
    close(SocketFD);

    return 0;
  }
