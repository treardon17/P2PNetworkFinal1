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
	std::set<std::string> knownServerIPs;
	std::vector<Socket*> connections;
	std::string myIP;
	const int port = 12345;

public:
	Client() {
		if (!Socket::Init()) {
			std::cerr << "Fail to initialize WinSock!\n";
			return;
		}
	}
	~Client(){}

	//adds a known IP to the Client's set of IPs
	void addKnownIP(std::string newIP) {
		knownServerIPs.insert(newIP);
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
			addKnownIP(peerIP);
			socket->msg_send(socket->getComputerIP()); //sends the client's IP address to the server node
		}
	}

	int done(const std::string message)
	{
		Socket::Cleanup();
		std::cout << message;
		std::cin.get();
		exit(0);
	}
};

#endif