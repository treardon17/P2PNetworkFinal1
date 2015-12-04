#ifndef __NODE_H__
#define __NODE_H__

#include<iostream>
#include <set>
#include <thread>
#include "Server.h"
#include "Client.h"

class Node {
private:
	std::set<std::string> *database;
	std::set<std::string> *knownIPs;
	Server *server;
	Client *client;

public:


	Node(){
		knownIPs = new std::set<std::string>;
		database = new std::set<std::string>;
		this->server = new Server(knownIPs, database); //start the server
		
		//get the ip address of the first computer to connect to from user
		std::string computerConnectIP;
		std::cout << "IP of computer to connect to: ";
		std::cin >> computerConnectIP;
		knownIPs->insert(computerConnectIP); //add that IP address to the list of known IPs

		Socket tempSock("tcp"); //to get current computer's id for client constructor
		
		this->client = new Client(knownIPs, server, tempSock.getComputerIP());
		runServer runS(server);
		runClient runC(client);
		std::thread server_thread(runS);	//let server run on an independant thread
		std::thread client_thread(runC);	//let client run on an independant thread
		server_thread.join();
		client_thread.join();
	}

	~Node(){
		delete server;
		server = NULL;
		delete client;
		client = NULL;
		delete database;
		database = NULL;
		delete knownIPs;
		knownIPs = NULL;
	}

	struct runServer {
		Server *server;
		runServer(Server *server) { this->server = server; }
		void operator()() {
			do {
				this->server->serverExecute();
			} while (true);
		}
	};

	struct runClient {
		Client *client;
		runClient(Client *client) { this->client = client; }
		void operator()() {
			do {
				this->client->query();
			} while (true);
		}
	};
};

#endif

