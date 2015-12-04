#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <iostream>
#include <set>
#include <string>
#include <sstream>
#include <vector>
#include "Socket.h"

class Client {
private:
	std::vector<Socket*> connections;
	std::set<std::string> *knownIPs;
	Server* server;
	std::string myIP;
	std::mutex lock;			// Lock console for mutual exclusive access
	const int port = 12345;

public:
	Client(std::set<std::string> *knownIPs, Server* server) {
		if (!Socket::Init()) {
			std::cerr << "Fail to initialize WinSock!\n";
			return;
		}
		this->knownIPs = knownIPs;
		this->server = server;
	}

	Client(const Client&client) {
		Server serve(*client.server);
		connections = client.connections;
		*knownIPs = *client.knownIPs;
		server = &serve;
		myIP = client.myIP;
	}

	~Client(){ }

	//adds a known IP to the Client's set of IPs
	void addKnownIP(std::string newIP) {
		std::lock_guard<std::mutex> lk(lock);
		knownIPs->insert(newIP);
	}

	//adds a socket connection to the vector of connections
	Socket* addConnection(std::string peerIP) {
		//create socket
		Socket* socket = new Socket("tcp");
		this->myIP = socket->getComputerIP();

		//connect socket to server
		if (!socket->sock_connect(peerIP, port)) {
			done("Could not connect to server");
		}
		else {
			std::cout << "Initializing handshake..." << std::endl;
			addKnownIP(peerIP);
			socket->msg_send(myIP); //sends the client's IP address to the server node
			{
				std::lock_guard<std::mutex> lk(lock);
				std::string allIPs = socket->msg_recv();
				if (allIPs != "") {
					std::cout << "Received response from peer!" << std::endl;
					std::vector<std::string> IPaddresses = getIPsFromString(allIPs);
					for (int i = 0; i < IPaddresses.size(); i++) {
						//don't add this computer's address to the known IPs (it already knows its own IP)
						if (IPaddresses[i] != myIP) { knownIPs->insert(IPaddresses[i]); }
					}//endfor
				}//endif
			}//endlock
		}//endelse

		connections.push_back(socket);
		return socket;
	}

	//allows a user to add a new connection
	void newConnection() {
		std::cout << "Connect to IP: ";
		std::string connectIP;
		std::cin >> connectIP;
		addConnection(connectIP);
	}

	std::vector<std::string> getIPsFromString(std::string IPstring) {
		std::vector<std::string> IPaddresses;
		std::string currentIP = "";
		for (int i = 0; i < IPstring.size(); i++) {
			if (IPstring[i] == ',') {
				IPaddresses.push_back(currentIP);
				currentIP = "";
			}
			else {
				currentIP += IPstring[i];
			}
		}
		return IPaddresses;
	}

	int done(const std::string message)
	{
		Socket::Cleanup();
		std::cout << message;
		std::cin.get();
		exit(0);
	}

	void query() {
		Socket *conn;
		if (knownIPs->size() > 0) {
			std::string connectIP = *knownIPs->begin();
			conn = addConnection(connectIP);
		}
		std::string query;
		std::cout << "Query for data: ";
		std::cin >> query;

		std::string msg = "";
		if (conn != NULL) {
			msg = conn->msg_recv();

			if (msg == "Ready for query") {
				conn->msg_send(query);
			}
			else {
				std::cout << "Incorrect server response\n";
				conn->sock_close();
			}

			msg = conn->msg_recv();

			if (msg == "Have it!!!")
				std::cout << "Successful Query\n";
			else
				std::cout << "Unsuccessful Query\n";

			conn->sock_close();
		}
	}

	void chooseAction() {
		std::vector<std::string> actions = { "Connect to additional peer", "Query for data" };
		std::cout << "Choose action to perform: " << std::endl;

		//output the list of potential actions
		for (int i = 0; i < actions.size(); i++) {
			std::cout << (char)(i + 97) << ") " << actions[i] << std::endl;
		}
		std::cout << "Choice: ";

		//error checking
		char choice = 'a';
		std::string chooseString = "";
		std::cin >> chooseString;
		if (chooseString.size() > 0) {
			choice = chooseString[0];
		}

		//choices
		switch (choice)
		{
		case 'a':
			newConnection();
			break;
		case 'b':
			query();
			break;
		default:
			std::cout << "Could not perform action. Invalid input." << std::endl;
			break;
		}
	}
};

#endif