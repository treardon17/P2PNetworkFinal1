#ifndef __NODE_H__
#define __NODE_H__

#include<iostream>
#include "Server.h"
#include "Client.h"

class Node {
private:
	Server* server;
	Client* client;

public:
	Node(){
		this->server = new Server();
		this->client = new Client();
	}
	~Node(){}

	Server* getServer() { return server; }
	Client* getClient() { return client; }
};

#endif

