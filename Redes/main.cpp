#include<iostream>
#include "Node.h"
#include "Server.h"
using namespace std;

int main (int argc, char *argv[]) {
	Server S;
	S.query("INSERT A {nombre:Jazmine,apellido:Alfaro,edad:20}");
	S.query("INSERT B {nombre:Renato,edad:21}");
	S.query("INSERT C {dni:99999999,edad:2}");
	return 0;
}

