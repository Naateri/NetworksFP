 /* Client code in C++ */
 // $ g++ -std=c++0x -o client.exe client.cpp

	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <string>
	#include <iostream>
	#include <thread> // std::thread, std::this_thread::sleep_for

	using namespace std;

	bool termino = false;
	char buffer[256];
	
	void requesting_access(int SocketFD, string identificador){
		string request="Slave requesting access "+identificador;
		int n = write(SocketFD, "Slave requesting access", 26);
		
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
	
	void read_thread(int socketCliente) 
	{
		int n;
		char buffer[1025];
		string str;
		do{
			n = read(socketCliente,buffer,1024);
			buffer[n]='\0';
			cout << endl << "(Remote Server:)" << buffer << endl << "MSG: ";
			str = buffer;
			if (str.compare("END")==0)
			termino=true;
		}while(!termino);
	}
	void write_thread(int socketCliente) 
	{
		int n;
		char buffer[1025];
		string str;
		do{
			cout << "MSG: ";
			getline(cin,str); 
			n = write(socketCliente,str.c_str(),str.size());
			if (str.compare("END")==0)
			termino=true;
		}while(!termino);
	}


	int main(void)
	{
	struct sockaddr_in stSockAddr;
	int Res;
	int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int n;

	if (-1 == SocketFD)
	{
	perror("cannot create socket");
	exit(EXIT_FAILURE);
	}

	memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(50001);
	Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr);

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

	if (-1 == connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
	{
	perror("connect failed");
	close(SocketFD);
	exit(EXIT_FAILURE);
	}
	
	string identificador;
	cout << "ingrese el identificador del slave: ";
	getline(cin,identificador); 
	
	requesting_access(SocketFD,identificador);
	
	std::thread t1(read_thread,SocketFD);
	std::thread t2(write_thread,SocketFD);
	
	t1.join();
	t2.join();

	while(!termino){};

	shutdown(SocketFD, SHUT_RDWR);

	close(SocketFD);
	return 0;
	}