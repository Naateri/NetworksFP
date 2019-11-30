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
#include <regex>
using namespace std;

int SocketFD;

const int l = 3;
std::string IP = "127.0.0.1";
int PORT = 40000;

bool end_connection = false;

string lrtrim(string str) {
	const std::string nothing = "" ;
	str = std::regex_replace( str, std::regex( "^\\s+" ), nothing ) ;
	str = std::regex_replace( str, std::regex( "\\s+$" ), nothing ) ;
	return str ; 
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

string slice_string(string &s){
	string delimiter = " ";
	int pos = s.find(delimiter);
	string s1 = s.substr(0, pos);
	s.erase(0, pos + delimiter.length());
	return s1;
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


void requesting_access(int SocketFD){
	//cout<<"Requesting access"<<endl;
	string request = size_string("Requesting access.");
	//cout<<request<<endl;
	write(SocketFD, request.c_str(), request.size());
	
	
	
	string str = make_read(SocketFD);
	//cout<<str<<endl;
	//slice_string(str);
	
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

	string str = make_read(SocketFD); // "OK."

	if (str == "OK."){

		string buffer1 = make_read(SocketFD);
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
	do{
		std::cout << "Type your message: ";
		string input = "" ; 
		std::getline (std::cin,input);  	
		if (input[0] == '0'){
			end_connection = true;
		}
		
		input = size_string(input);
		//cout<<"INPUT "<<input<<endl;
		write(SocketFD, input.c_str(), input.size()); //cuantos bytes estoy mandando
		
	} while(!end_connection);
}

void rcv_msg(){
	do{	
		string buffer = make_read(SocketFD);
		string aux = buffer;
		string verify_select = slice_string(aux);
		if(verify_select == "sq"){
			vector<string> select = separate_string(aux, "/");
			for(uint i=0;i<select.size();++i){
				cout<<select[i]<<endl;
			}
		}
		else{
		//if (n < 0) perror("ERROR reading from socket");
			cout<<"Server: ["<<buffer<<"]"<<endl;
		}
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
