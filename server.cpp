 /* Server code in C */
 //$ g++ -std=c++0x -o tarea.exe tarea.cpp
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <iostream>
  #include <map>
  #include <vector>
  #include <thread>

		
	
	bool termino = false;
	
	std::map<int,int> slaves;

	void write_thread(int socketCliente,char buffer[1025],int tam) 
	{
		int n;
		//char buffer[1025];
		std::string str;
		n = write(socketCliente,buffer,tam);
	}
	void rcv_msg(int socketCliente) 
	{
		int n;
		char buffer[1025];
		std::string str;
		do{

		}while(!termino);
	}
	
	bool confirming_connection_slave(int ConnectFD)
	{
		char buffer[256];
			
		int n= read(ConnectFD,buffer,23);  
		buffer[n]='\0';
		
		if(strcmp("Slave requesting access", buffer) == 0){
			n= read(ConnectFD,buffer,2);				
			buffer[n]='\0';
	
			int identificador = atoi(buffer);
			std::map<int,int>::iterator it;
			it = slaves.find(identificador);
			if (it != slaves.end()){
				n = write(ConnectFD, "No.", 3);
				return false;
			}
			else{
				slaves[identificador]=ConnectFD;
				std::thread (rcv_msg,ConnectFD).detach();
				//cout<<"esclavo ingresado"<<endl;
				n = write(ConnectFD, "OK.", 3);
				
				n = read(ConnectFD, buffer, 4);
				return true;
			}
		}
		else{
				n = write(ConnectFD, "No.", 3);
				return false;
			}
	}
				
	
	
  int main(void)
  {
    sockaddr_in stSockAddr;
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    char buffer[1025];
    int n;
	
 
    if(-1 == SocketFD)
    {
      perror("can not create socket");
      exit(EXIT_FAILURE);
    }
 
    memset(&stSockAddr, 0, sizeof(sockaddr_in));
 
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(50001);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;
 
    if(-1 == bind(SocketFD,(const sockaddr *)&stSockAddr, sizeof(sockaddr_in)))
    {
      perror("error bind failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
 
    if(-1 == listen(SocketFD, 10))
    {
      perror("error listen failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
 
    for(;;){
		while(!termino){		
			int ConnectFD  = accept(SocketFD, NULL, NULL);
			
			bool connection =confirming_connection_slave(ConnectFD);
			
			if(connection){
				std::thread t1(rcv_msg, ConnectFD);
				t1.detach();
			}
			else {
			shutdown(ConnectFD, SHUT_RDWR);
			close(ConnectFD);
			}
			
			
			
		}
	}
    close(SocketFD);
    return 0;
  }