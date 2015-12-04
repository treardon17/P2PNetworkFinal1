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

		//get the ip address of the first computer to connect to from user
		std::string computerConnectIP;
		std::cout << "IP of computer to connect to: ";
		std::cin >> computerConnectIP;
		knownIPs->insert(computerConnectIP);			//add that IP address to the list of known IPs


		//NEED TO MAKE CONNECTION HERE SO THAT IT DOESN'T HANG UP...
		//OR DETACH THREAD


		//server thread
		this->server = new Server(knownIPs, database);	//start the server
		runServer runS(server);
		std::thread server_thread(runS);				//let server run on an independant thread

		//client thread
		this->client = new Client(knownIPs, server);
		runClient runC(client);
		std::thread client_thread(runC);				//let client run on an independant thread

		client_thread.join();
		server_thread.join();
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
			this->server->serverExecute();
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

