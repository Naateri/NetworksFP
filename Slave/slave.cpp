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
#include <fstream>
#include <vector>
#include <algorithm>
#include <regex>
#include <signal.h>

#define MAX_SLAVES 2
#define uint unsigned int

using namespace std;

int SocketFD;

int SocketFD1;
char buffer[256];
int ConnectFD;
std::string IP = "127.0.0.1";
int PORT = 40000;
int PORTSLAVE = 50007;

bool end_connection = false;
const int l = 3;

string lrtrim(string str) {
	const std::string nothing = "" ;
	str = std::regex_replace( str, std::regex( "^\\s+" ), nothing ) ;
	str = std::regex_replace( str, std::regex( "\\s+$" ), nothing ) ;
	return str ; 
}


string size_string(string s){
	int num = s.size();
	num += 1; // " "
	//num += l+1;
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
	//cout<<"Make read"<<endl;
	char size[l];
	read(fd,size,l);
	int len = atoi(size);
	
	//cout<<"TAM"<<len<<endl;
	char *buffer = new char [len];
	int n = read(fd,buffer,len);
	//buffer[n] = '\n';
	string str(buffer); 
	
	
	str = lrtrim(str);
	str.resize(len-1);
//	cout<<"STRING: |"<<str<<"|"<<endl;
	return str;

	/*char size[l];
	read(fd,size,l);
	int len = atoi(size);
	//cout<<len<<endl;
	char *buffer = new char [len];
	read(fd,buffer,len);
	string str(buffer); 
	slice_string(str);
	return str;*/
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
	
vector<string> separate_attributes(string val){
	std::string delimiter1 = "{}";
	vector<string> res;
	
	for (char c: delimiter1) {
		val.erase(std::remove(val.begin(), val.end(), c), val.end());
	}
	if(val.size()==0){
		cout<<"No attributes"<<endl;
	}
	else{
		vector<string> valores = separate_string(val,",");
		
		for(uint i=0;i<valores.size();++i){
			vector<string> v = separate_string(valores[i],":");
			res.push_back(v[0]);
			res.push_back(v[1]);
		}
	}
	return res;
}
	
int hash_function(std::string value){
	int cur_sum = 0;
	for (uint i = 0; i < value.size(); i++){
		cur_sum += (int)value[i];
	}
	return cur_sum % MAX_SLAVES + 1;
}
	
string backup()
{
	std::fstream file;
	string slave_txt = "slave.txt";
	file.open(slave_txt, ios::in);
	string line;
	string tempRes;
	while ( getline (file,line) && !file.eof()){
		tempRes += line + " / ";
	}
	tempRes.resize(tempRes.size()-2);
	
	file.close();
	tempRes="backup "+tempRes;
	tempRes = size_string(tempRes);
	return tempRes;
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
		delSpaces(value);
		return hash_function(value);
	} else return 0;
}

string insert_node(string s){
	
	std::string return_to_server = "Node was inserted"; //result to be sent to client
	string node_id = slice_string(s);
	//cout<<"NODE ID: " <<node_id<<endl;
	//cout<<"RESTO: "<<s<<endl;
	vector<string> attr;
	
	if(s.size()>5){/// 2 minimo {a:c}
		attr = separate_attributes(s);
		for(uint i=0;i<attr.size();++i){
			attr[i] = lrtrim(attr[i]);
			//cout<<"|"<<attr[i]<<"|"<<endl;
		}
	}
	else{
		attr.resize(0);
	}
	
	/*
	for(uint i=0;i<attr.size();++i){
		cout<<"ATTRIBUTE:|"<<attr[i]<<"|"<<endl;
		cout<<"VALUE:|"<<attr[i+1]<<"|"<<endl;
		++i;
		
	}
	*/
	
	int hash = hash_function(node_id);
	
	
	///string slave_txt = "slave_" + to_string(hash)+ ".txt";
	
	string slave_txt = "slave.txt";
	
	//cout<<slave_txt<<endl;
	ofstream fs;
	fs.open(slave_txt, ios::app);
	fs.seekp(0, ios::end);
	if(fs.is_open()){
		
		fs <<endl;
		fs<< node_id<<endl;
		for(uint i=0;i<attr.size();i+=2){
			//cout<<"I: "<<i<<endl;
			fs<<attr[i]<<" : "<<attr[i+1]<<endl;
		}
	}
	

	fs.close();	
	
	return return_to_server;
	
}
	
string insert_adjacency(string s){
	vector<string> nodes = separate_string(s , " ");
	
	
	string rel =nodes[0] +" - "+nodes[1];

	int num_file = hash_function(nodes[0]);
	

	
	std::fstream myfile, outfile;
	
	///string slave_txt = "slave_" + to_string(num_file)+ ".txt";
	
	string slave_txt = "slave.txt";
	
	//cout<<slave_txt<<endl;
	myfile.open(slave_txt, ios::in);
	
	string line;
	
	vector<string> txt;
	txt.push_back(rel);
	//cout<<"INSERT_RELATION"<<endl;
	bool isInserted = false;
	if (myfile.is_open()){
		while ( getline (myfile,line) && !myfile.eof()){
			txt.push_back(line);
			if(line == nodes[0]){
				isInserted = true;
			}
		}
		myfile.close();
		
	}
	

	
	outfile.open(slave_txt, ios::out );
	outfile.clear();
	outfile.seekp(0);
	
	for(uint i=0;i<txt.size();++i){
		//cout<<txt[i]<<endl;
		outfile <<txt[i]<<endl;
	}
	
	outfile.close();
	
	if(!isInserted){
		string toInsert = nodes[0] + " {}";
		insert_node(toInsert);
	}
	
	return "Adjacency was inserted";
}
	
string all_adjacencies(string msg){
	vector<string> info = separate_string(msg, " ");
	string fd = info[1];
	delSpaces(info[0]);
	string node_id = info[0];
	/*
	delSpaces(msg);
	string node_id = msg; */
	int num_file = hash_function(msg);
	
	std::fstream myfile;
	
	//string slave_txt = "slave_" + to_string(num_file)+ ".txt";
	string slave_txt = "slave.txt";
	
	myfile.open(slave_txt, ios::in);
	
	string line;
	
	vector<string> txt;
	txt.push_back(node_id);
	//cout<<"INSERT_RELATION"<<endl;
	bool isInserted = false;
	if (myfile.is_open()){
		while ( getline (myfile,line) && !myfile.eof()){
			if(line == "\0"){
				break;
			}
			vector<string> adj = separate_string(line," "); 
			delSpaces(adj[0]);
			//string aux = line.substr(0, node_id.size()); 
			if(adj[0] == node_id){
				delSpaces(adj[2]);
				txt.push_back(adj[2]);
			}
		/*
			string aux = line.substr(0, node_id.size()); 
			if(aux == node_id){
				int tam = node_id.size(); 
				txt.push_back(line.substr(tam+3, line.size()-tam-3));
			}
		*/
		}
		myfile.close();
		
	}
	
	string all_adjacencies = "delete" ;	
	for(int i=0;i<txt.size();++i){
		all_adjacencies += " " + txt[i];
	}

	all_adjacencies += ' ' + fd;
	all_adjacencies = size_string(all_adjacencies);
	//cout<<"Mandando: "<<all_adjacencies<<endl;
	return all_adjacencies ;
	
}
	
string delete_adjacency(string s){
	vector<string> nodes = separate_string(s , " ");
	string rel =nodes[0] +" - "+nodes[1];
	
	int num_file = hash_function(nodes[0]);
	
	std::fstream myfile, outfile;
	//string slave_txt = "slave_" + to_string(num_file)+ ".txt";
	
	string slave_txt = "slave.txt";
	//cout<<slave_txt<<endl;
	myfile.open(slave_txt, ios::in);
	
	string line;
	
	vector<string> txt;

	if (myfile.is_open()){
		while ( getline (myfile,line) && !myfile.eof()){
		
			if(line != rel){
				txt.push_back(line);
			}
			
		}
		myfile.close();
	}
	
	outfile.open(slave_txt, ios::out );
	outfile.clear();
	outfile.seekp(0);
	
	for(uint i=0;i<txt.size();++i){
		cout<<txt[i]<<endl;
		outfile <<txt[i]<<endl;
	}
	
	outfile.close();
	
	return "Adjacency was deleted";
}
	
void delete_node(string node){
	
	std::size_t found = node.find('\0');
	
	node = node.substr(0,found);
	
	delSpaces(node);
	
	std::fstream myfile, outfile;
	
	//string slave_txt = "slave_" + to_string(num_file)+ ".txt";
	
	string slave_txt = "slave.txt";
	
	//cout<<slave_txt<<endl;
	myfile.open(slave_txt, ios::in);
	
	string line;
	
	vector<string> txt;
	bool begin = false;
	bool end = false;
	if (myfile.is_open()){
		while ( getline (myfile,line) && !myfile.eof()){
			//delSpaces(line);
			//cout<<"Leyendo: |"<<line<<"|"<<endl;
			
			if(line == node){
				begin = true;
			}
			
			else if(begin && line == "" && !end){
				end = true;
				continue;
			}
			
			if(begin==true && end == true){
				txt.push_back(line);
			}
			else if(begin==true){
				;
			}
			else if(!begin){
				txt.push_back(line);
			}
			else
				txt.push_back(line);
		/* //BRANCH MANUEL
			else if(begin==true){
				;
			}
			else if(begin == true && line == "" ){
				end = true;
			}
			else if(begin==true && end == true){
				txt.push_back(line);
			}
			else
			   txt.push_back(line);
		*/
		}
		myfile.close();
	}
}
	

std::string select(std::string msg){
	
	//// string msg_slave = "server 2 " + node_id + ' ' + to_string(lvl) + ' ' +to_string(CId);
	//cout<<"MENSAJE: "<<msg<<endl;
	
	std::fstream file;

	string slave_txt = "slave.txt";

	//string slave_txt = "backup.txt";

	file.open(slave_txt, ios::in);
	string line;
	string tempRes;
	vector<string> separate = separate_string(msg, " ");
	string CID = separate[2];
	
	
	
	bool attributes = 0;
	bool findAttibutes = 0;
	bool once = 1;
	bool put_in_msg = false;
	
	
	if(separate[1] == "1"){
		while ( getline (file,line) && !file.eof()){
			
			if(line == "" && once){
				
				tempRes+= "|/Attributes: / ";
				attributes = 1;
				once = 0;
			}
			
			if(!attributes){
				vector<string> n = separate_string(line, " ");
				if(separate[0] == n[0]){ /// adjacencies
					//tempRes+= line + " == ";
					tempRes += line + " / ";
					//vector<string> temp = separate_string(line, " ");
					//nodes.push_back(temp[temp.size()-1]);
				}
			}
			else{
				if(line == separate[0]){
					put_in_msg = true; 
					continue;
				}
				if(put_in_msg && line == ""){
					break;
				}
				if(put_in_msg ){
					tempRes += line + "/ ";
				}
			}
			
		}
	
		tempRes.resize(tempRes.size()-2);
	
		//string lengthString = to_string(tempRes.size());
	/*	while(lengthString.size()<6){
			lengthString = "0"+lengthString;
		}
		*/
		tempRes =  "s "+CID+" "+tempRes;
		cout<<"TempRes: "<<tempRes<<endl;
		file.close();
		
	}
	//cout<<"res 1 "<<tempRes<<endl;
	tempRes = size_string(tempRes);
	return tempRes;
}

void update_node(string node){ //UPDATE <NODE> ATTR atrribute:value
	
	/*std::size_t found = node.find(' ');
	
	node = node.substr(0,found);*/
	
	string attribute, value;
	string real_node = slice_string(node);
	
	cout << "real_node: " << real_node << endl;
	
	slice_string(node); //Attr
	
	cout << "node: " << node << endl; //ATTR
	
	attribute = slice_string(node);
	vector<string> attributes = separate_string(attribute, ":");
	attribute = attributes[0]; value = attributes[1];
	
	cout << "attribute " << attribute << ", value " << value << endl;
	
	delSpaces(real_node);
	delSpaces(attribute); delSpaces(value);
	
	std::fstream myfile, outfile;
	
	string slave_txt = "slave.txt";
	
	myfile.open(slave_txt, ios::in);
	
	string line, temp;
	
	vector<string> txt;
	bool begin = false;
	bool end = false;
	bool updated = false;
	bool found_node = false;
	if (myfile.is_open()){
		while ( getline (myfile,line) && !myfile.eof()){
			//delSpaces(line);
			cout<<"Leyendo: |"<<line<<"|"<<endl;
			
			if(line == real_node){
				begin = true;
				found_node = true;
				txt.push_back(line);
			}
			else if(begin==true){
				cout << "line.substr " << line.substr(0, attribute.size()) << endl;
				if (line.substr(0, attribute.size()) == attribute){
					temp = attribute + " : " + value;
					txt.push_back(temp);
					updated = true;
				} else txt.push_back(line);
			}
			else if(begin == true && line == "" ){
				if (updated == true){
					txt.push_back(line);
				} else {
					temp = attribute + " : " + value;
					txt.push_back(temp);
				}
				end = true;
			}
			else if(begin==true && end == true){
				txt.push_back(line);
			}
			else
				txt.push_back(line);			
		}
		myfile.close();
	}
	
	if (updated == false && found_node == true){
		temp = attribute + " : " + value;
		txt.push_back(temp);
	}
	
	outfile.open(slave_txt, ios::out );
	outfile.clear();
	outfile.seekp(0);
	
	for(uint i=0;i<txt.size();++i){
		cout<<txt[i]<<endl;
		outfile <<txt[i]<<endl;
	}
	
	outfile.close();
	
}

void keepalive(){
	string respuesta="008 1024";
	write(SocketFD, respuesta.c_str(), respuesta.size());
	cout<<"sent"<<endl;
}
	
void parse_message(string msg){
	string result;
	
	string type_query = slice_string(msg);
	transform(type_query.begin(), type_query.end(),type_query.begin(), ::tolower);
	delSpaces(type_query);
	//cout<<"Type Query: "<<type_query<<endl;
	
	if(type_query == "0"){
		cout<<"Inserting node"<<endl;
		result = insert_node(msg);
		result = size_string(result);
		write(SocketFD, result.c_str(), result.size());

		string b=backup();
		write(SocketFD, b.c_str(), b.size());
	
	}
	else if(type_query == "1"){
		cout<<"Inserting adjacency"<<endl;
		result = insert_adjacency(msg);
		result = size_string(result);
		write(SocketFD, result.c_str(), result.size());
		
		string b=backup();
		write(SocketFD, b.c_str(), b.size());
	}
	else if(type_query == "2"){
		cout<<"Select"<<endl;
		result = select(msg);
		result = size_string(result);
		write(SocketFD, result.c_str(), result.size());
	} 
	else if(type_query == "3"){
		cout<<"Delete node"<<endl;
		delete_node(msg);
	
		string b=backup();
		write(SocketFD, b.c_str(), b.size());
	} 
	else if(type_query == "adj"){
		cout<<"Requesting all adjacencies of a node"<<endl;
		result = all_adjacencies(msg);
		result = size_string(result);
		write(SocketFD, result.c_str(), result.size());

		string b=backup();
		write(SocketFD, b.c_str(), b.size());
	}
	else if(type_query == "4"){
		cout<<"Delete adjacency"<<endl;
		delete_adjacency(msg);
	} else if (type_query == "9"){
		cout << "Update attribute"<< endl;
		update_node(msg);
	}
/*
		string b=backup();
		write(SocketFD, b.c_str(), b.size()); */
	else if(type_query[0] == '5'){
		cout<<"Keep alive"<<endl;
		keepalive();
		
	} 
	else {
		result = "Error. Query not understood\n";
		result = size_string(result);
		write(SocketFD, result.c_str(), result.size());
	}
	
}

void requesting_access(int SocketFD, string identificador){ //new requesting access
	string request="Slave requesting access "+identificador;
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

		printf("Connection to database as a slave established.\n");
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
		
		input = (input);
		write(SocketFD, input.c_str(), input.size()); //cuantos bytes estoy mandando
		
	} while(!end_connection);
}

	
		
void rcv_msg(){
	
	int n;
	do{	
		string temp = make_read(SocketFD);
		
		string result;
		if (n < 0) perror("ERROR reading from socket");
		
		if (temp.substr(0, 6) == "server"){
			
			slice_string(temp);
			//cout<<"Mensaje de server"<<endl;
			//cout<<temp<<endl;
			parse_message(temp);
		}

		//cout<<"Server: [" << temp <<"]"<<endl;
		
		/*
			cout<<"Mensaje de server"<<endl;
			cout<<temp<<endl;
			parse_message(temp);
		
		}*/
		
		if (temp.substr(0, 6) == "backup"){
			cout<<"Mensaje de backup"<<endl;
			string id = to_string(get_id());
			id = size_string(id);
			write(SocketFD, id.c_str(), id.size());
		}
		
		printf("Server: [%s]\n",buffer);
	} while(!end_connection);
	
	end_connection = true;
}

void sighandler(int signum) 
{	
	if(signum == 2)
	{
		
		string close = size_string("Closing Connection.");
		write(SocketFD, close.c_str(), close.size());
		
		//shutdown(SocketFD, SHUT_RDWR);
		
		//close(SocketFD);
	}
}

			
int main(void){
	cout << "SLAVE"<<endl;

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
	
	requesting_access(SocketFD, id);
	
	//std::thread t1(send_msg);
	std::thread t2(rcv_msg);
	
	//t1.join();
	t2.join();

	shutdown(SocketFD, SHUT_RDWR);
	
	close(SocketFD);
	return 0;
}
