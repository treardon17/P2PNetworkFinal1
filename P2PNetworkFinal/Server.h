#ifndef __SERVER_H__
#define __SERVER_H__

#include<iostream>
#include <string>
#include <iterator>
#include <set>
#include <sstream>
#include <thread>
#include <mutex>
#include "Socket.h"

class Server {
private:
	std::set<std::string> *knownIPs;
	std::set<std::string> *database;
	Socket* socket;
	std::mutex lock;					// Lock console for mutual exclusive access
	bool receivedIPFromClient;
	const int port = 12345;

public:

	int done(const std::string message)
	{
		Socket::Cleanup();
		std::cout << message;
		std::cin.get();
		exit(0);
	}

	Server(std::set<std::string> *knownIPs, std::set<std::string> *database) {
		this->knownIPs = knownIPs;
		this->database = database;
		this->receivedIPFromClient = false;

		if (!Socket::Init()) {
			std::cerr << "Fail to initialize WinSock!\n";
			return;
		}

		this->socket = new Socket("tcp"); //create tcp socket

		//socket bound to port
		if (!socket->sock_bind("", port)) {
			std::cerr << "Could not bind to port number " << port;
			done("Error binding port.");
		}

		//socket is listening
		if (!socket->sock_listen(1)) {
			done("Could not get socket to listen.");
		}

		populateDatabase();
	}

	//Copy constructor
	Server(const Server&server) {
		*knownIPs = *server.knownIPs;
		*database = *server.database;
	}

	//Destructor
	~Server() {
		delete socket;
		socket = NULL;
	}

	void populateDatabase() {
		std::lock_guard<std::mutex> lk(lock);
		for (int i = 0; i < 10; i++) {
			std::ostringstream myInt;
			myInt << i;
			database->insert(myInt.str());
		}
	}

	void serverExecute(){
		Socket *conn;
		do {
			conn = socket->sock_accept();
			Connect myConnection(conn, this);
			std::thread *thr = new std::thread(myConnection);
			thr->detach();
		} while (true);
	}

	//Allows server to send the connected client all IP addresses that have connected to the server
	void sendClientAllIPs(Socket* conn) {
		std::ostringstream ipStream; //stream that takes in the ip addresses
		std::set<std::string>::iterator setItr;

		//ensure at least this computer's IP address is in the knownIPs
		if (knownIPs->size() == 0) {
			std::lock_guard<std::mutex> lk(lock);
			knownIPs->insert(conn->getComputerIP());
		}

		//formatting the IPs separated by commas
		for (setItr = knownIPs->begin(); setItr != knownIPs->end(); setItr++) {
			ipStream << *setItr << ",";
		}
		conn->msg_send(ipStream.str());			//send the client the formatted IP string
	}

	static void serverListen(Socket* conn, Server* server) {
		std::string msg;
		do {
			msg = conn->msg_recv();
			//std::cout << msg << std::endl; //debugging
			if (!server->receivedIPFromClient) {
				std::mutex lock;
				std::lock_guard<std::mutex> lk(lock);
				server->sendClientAllIPs(conn);
				server->knownIPs->insert(msg);
				//server->receivedIPFromClient = true;
			}

			msg = "Ready for query.";
			conn->msg_send(msg);	//server response

			msg = conn->msg_recv(); // client query
			bool found = false;

			if (msg != "") {
				std::set<std::string>::iterator it;
				for (it = server->database->begin(); it != server->database->end(); it++) {
					if (*it == msg) {
						conn->msg_send("Have it!!!");
						found = true;
						break;
					}
				}
			}

			//if the data was not found
			if (!found) {
				conn->msg_send("Don't have it.");
			}

		} while (msg != "");
	}

	struct Connect {
		Socket* conn;
		Server* myServer;
		Connect(Socket*conn, Server* myServer) { 
			this->conn = conn; 
			this->myServer = myServer;
		}
		void operator()() {
			Server::serverListen(this->conn, this->myServer);
		}
	};
};


#endif