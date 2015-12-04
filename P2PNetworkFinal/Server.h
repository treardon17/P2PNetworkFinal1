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
	std::mutex lock;			// Lock console for mutual exclusive access
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
	}

	Server(const Server&server) {
		*knownIPs = *server.knownIPs;
		*database = *server.database;
	}

	//Destructor
	~Server() {
		delete socket;
		socket = NULL;
	}

	/*
	void serverExecute(){
		Socket *conn;
		do {
			conn = socket->sock_accept();
			Node *node = new Node(conn);
			std::thread *thr = new std::thread(*node);
			thr->detach();
		} while (true);
	}
	*/

	//Allows server to send the connected client all IP addresses that have connected to the server
	std::string sendClientAllIPs() {
		std::ostringstream ipStream; //stream that takes in the ip addresses
		std::set<std::string>::iterator setItr;
		//formatting the IPs separated by commas
		for (setItr = knownIPs->begin(); setItr != knownIPs->end(); setItr++) {
			ipStream << *setItr << ",";
		}
		Socket conn = *socket->sock_accept();
		conn.msg_send(ipStream.str()); //send the client the formatted IP string
	}

	//DO WE NEED THIS???
	void operator()() {
		std::string msg;
		std::cout << "server operator works!" << std::endl;
		//do {
			//msg = socket->msg_recv();

			//NOTE: Check message, then query the database for data in the message, then send results

		//} while (msg != "");
	}
};


#endif