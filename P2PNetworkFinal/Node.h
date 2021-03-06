#ifndef __NODE_H__
#define __NODE_H__

#include <iostream>
#include <set>
#include <thread>
#include "Server.h"
#include "Client.h"

class Node {
private:
	std::set<std::string> *knownIPs;
	Server *server;
	Client *client;
	sqlite3* localDatabase;

public:

	Node(){
		knownIPs = new std::set<std::string>;
		//database = new std::set<std::string>;

		char *zErrMsg = 0;
		int rc;

		rc = sqlite3_open("localDB.db", &localDatabase);

		if (rc) {
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(localDatabase));
			exit(0);
		}
		else {
			fprintf(stderr, "Opened database successfully\n");
		}

		//server thread
		this->server = new Server(knownIPs, localDatabase);	//start the server
		runServer runS(server);								//functor to be run in the server thread
		std::thread server_thread(runS);					//let server run on an independant thread

		//client thread
		this->client = new Client(knownIPs, server);	//start the client
		runClient runC(client);							//functor to be run in the client thread
		std::thread client_thread(runC);				//let client run on an independant thread

		client_thread.join();							//sync the threads up
		server_thread.join();							//
	}

	//deconstructor
	~Node(){
		delete server;
		server = NULL;
		delete client;
		client = NULL;
		//delete database;
		//database = NULL;
		delete knownIPs;
		knownIPs = NULL;

		//close the database and delete it
		sqlite3_close(localDatabase);
		delete localDatabase;
		localDatabase = NULL;
	}

	//server functor run in thread
	struct runServer {
		Server *server;
		runServer(Server *server) { this->server = server; }
		void operator()() {
			this->server->serverExecute();
		}
	};

	//client functor run in thread
	struct runClient {
		Client *client;
		runClient(Client *client) { this->client = client; }
		void operator()() {
			do {
				this->client->chooseAction();
			} while (true);
		}
	};
};

#endif

