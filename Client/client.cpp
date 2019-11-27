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

const int l = 3;
std::string IP = "127.0.0.1";
int PORT = 40005;

bool end_connection = false;

string slice_string(string &s){
	string delimiter = " ";
	int pos = s.find(delimiter);
	string s1 = s.substr(0, pos);
	s.erase(0, pos + delimiter.length());
	return s1;
}
	
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


void requesting_access(int SocketFD){
	//cout<<"Requesting access"<<endl;
	string request = size_string("Requesting access.");
	//cout<<request<<endl;
	write(SocketFD, request.c_str(), request.size());
	
	
	char size[l];
	read(SocketFD,size,l);
	int len = atoi(size);
	//cout<<len<<endl;
	
	char *buffer = new char [len];
	int n = read(SocketFD,buffer,len);
	
	
	string str(buffer);
	//cout<<str<<endl;
	slice_string(str);
	
	if (str != "OK."){
		cout<<"Erroneous confirmation. Ending connection"<<endl;
		shutdown(SocketFD, SHUT_RDWR);
		close(SocketFD);
	} else {
		string msg = size_string("OK.");
		write(SocketFD, msg.c_str(), msg.size());
		cout<<"Connection to database established"<<endl;
	}
}

void closing_connection(){
	
	string close = size_string("Closing Connection.");
	write(SocketFD, close.c_str(), close.size());

	char size[l];
	read(SocketFD,size,l);
	int len = atoi(size);
//	cout<<len<<endl;
	char *buffer = new char [len];
	read(SocketFD,buffer,len);
	
	string str(buffer); // "OK."
	slice_string(str);
	if (str == "OK."){
		
		char size[l];
		read(SocketFD,size,l);
		int len = atoi(size);
		//cout<<len<<endl;
		char *buffer1 = new char [len];
		read(SocketFD,buffer1,len);
		string str1(buffer1); //Are you sure?
		slice_string(str1);
		if (buffer1 == "Are you sure?"){
			string msg = size_string("Yes.");
			write(SocketFD, msg.c_str(), msg.size());
			printf("Ending connection with server. Closing socket.\n");
		} 
		else
			printf("Erroneous confirmation. Server not asking.\n");
	} 
	else {
		printf("Erroneous confirmation. Server not confirming.\n");
	}
	
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
		char size[l];
		read(SocketFD,size,l);
		int len = atoi(size);
		//cout<<len<<endl;
		char *buffer = new char [len];
		n = read(SocketFD,buffer,len);
		
		if (n < 0) perror("ERROR reading from socket");
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
