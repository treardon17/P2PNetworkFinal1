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
	Client(std::set<std::string> *knownIPs, Server* server, std::string myIP) {
		if (!Socket::Init()) {
			std::cerr << "Fail to initialize WinSock!\n";
			return;
		}
		this->knownIPs = knownIPs;
		this->server = server;
		this->myIP = myIP;

		//add a connection with the IP address that was given by the user
		if (knownIPs->size() > 0) {
			std::string connectIP = *knownIPs->begin();
			addConnection(connectIP);
		}
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
	void addConnection(std::string peerIP) {
		//create socket
		Socket* socket = new Socket("tcp");
		//connect socket to server
		if (!socket->sock_connect(peerIP, port)) {
			done("Could not connect to server");
		}
		else {
			std::cout << "Connection made!" << std::endl;
			addKnownIP(peerIP);
			socket->msg_send(myIP); //sends the client's IP address to the server node
			{
				std::lock_guard<std::mutex> lk(lock);
				std::vector<std::string> IPaddresses = getIPsFromString(socket->msg_recv());
				for (int i = 0; i < IPaddresses.size(); i++) {
					knownIPs->insert(IPaddresses[i]);
				}
			}
		}
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
		
		//std::string query;
		//std::cout << "Query for data: ";
		//std::cin >> query;
	}
};

#endif