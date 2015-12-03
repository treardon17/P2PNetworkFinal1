#ifndef __NODE_H__
#define __NODE_H__

#include<iostream>
#include <set>
#include <thread>
#include "Server.h"
#include "Client.h"

class Node {
private:
	Server *server;
	Client *client;
	std::set<std::string> *database;
	std::set<std::string> *knownIPs;

public:
	Node(){
		//get the ip address of the first computer to connect to from user
		std::string computerConnectIP;
		std::cout << "IP of computer to connect to: ";
		std::cin >> computerConnectIP;
		knownIPs->insert(computerConnectIP); //add that IP address to the list of known IPs

		Socket tempSock("tcp"); //to get current computer's id for client constructor
		this->server = new Server(knownIPs, database); 
		this->client = new Client(knownIPs, database, tempSock.getComputerIP());
		std::thread server_thread(*server);	//let server run on an independant thread
		std::thread client_thread(*client);	//let client run on an independant thread
		server_thread.join();
		client_thread.join();
	}

	~Node(){
		delete server;
		delete client;
		delete database;
		delete knownIPs;
	}
};

#endif

