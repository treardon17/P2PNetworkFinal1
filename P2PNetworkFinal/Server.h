#ifndef __SERVER_H__
#define __SERVER_H__

#include <iostream>
#include <string>
#include <iterator>
#include <set>
#include <sstream>
#include <thread>
#include <mutex>
#include "SQLite\sqlite3.h"
#include "Socket.h"

//DATABASE CONNECTION LIBRARIES

//END DATABASE CONNECTION

class Server {
private:
	std::set<std::string> *knownIPs;
	sqlite3* localDatabase;
	Socket* socket;
	std::mutex lock;					// Lock console for mutual exclusive access
	const int port = 12345;

public:

	int done(const std::string message)
	{
		Socket::Cleanup();
		std::cout << message;
		std::cin.get();
		exit(0);
	}

	Server(std::set<std::string> *knownIPs, sqlite3 *localDatabase) {
		this->knownIPs = knownIPs;
		this->localDatabase = localDatabase;

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
	}

	//Destructor
	~Server() {
		delete socket;
		socket = NULL;
	}

	static int callback(void *data, int argc, char **argv, char **azColName) {
		//int i;
		//fprintf(stderr, "%s: ", (const char*)data);
		//for (i = 0; i<argc; i++) {
			//printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		//}
		//printf("\n");
		return 0;
	}

	//makes the DATA table in the localDatabase
	bool makeTable() {
		//SQL to make table
		std::string sql = "CREATE TABLE DATA("  \
			"ID INTEGER PRIMARY KEY AUTOINCREMENT	NOT NULL," \
			"NAME           TEXT					NOT NULL," \
			"AGE            INT						NOT NULL);";

		//executes SQL statement
		if (executeSQL(sql) != "error") {
			std::cout << "SERVER: created table" << std::endl;
			return true;
		}
		else {
			std::cout << "SERVER: failed to create table" << std::endl;
			return false;
		}
	}


	//checks to see if the DATA table exists
	std::string tableExists() {
		std::string sql = "SELECT * FROM DATA";
		return executeSQL(sql);
	}


	//executes query on local database
	std::string executeSQL(std::string sql) {
		char *zErrMsg = 0;
		const char* data = "Callback function called";
		int rc;

		/*
		rc = sqlite3_exec(localDatabase, strdup(sql.c_str()), callback, 0, &zErrMsg);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
			return "error";
		}
		else*/ {
			sqlite3_free(zErrMsg);
			std::string results;

			//get all the rows
			sqlite3_stmt *statement;
			int res = 0;

			if (sqlite3_prepare(localDatabase, strdup(sql.c_str()), -1, &statement, 0) == SQLITE_OK)
			{
				int ctotal = sqlite3_column_count(statement);

				while (1){
					res = sqlite3_step(statement);
					if (res == SQLITE_ROW){
						for (int i = 0; i < ctotal; i++){
							results += (char*)sqlite3_column_text(statement, i);
							results += "\n";
						}
					}
					if (res == SQLITE_DONE || res == SQLITE_ERROR){
						break;
					}
				}
			}

			if (res != SQLITE_DONE) {
				fprintf(stderr, "SQL error: %s\n", zErrMsg);
				sqlite3_free(zErrMsg);
				return "error";
			}

			//end get all rows

			return results; //success
		}
		//return "error"; //fail
	}

	void populateDatabase() {
		std::lock_guard<std::mutex> lk(lock);

		//INSERT STUFF INTO THE SQLITE DATABASE!!
		bool tableExists = false;

		if (this->tableExists() != "error") {
			tableExists = true;
		}
		else {
			//make the table and see if it was successful
			tableExists = makeTable();
		}

		//if the table exists, then we can add stuff to it
		if (tableExists) {
			
			//populate database here!!!

		}
		else {
			std::cout << "Could not populate database." << std::endl;
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

			std::mutex lock;
			std::lock_guard<std::mutex> lk(lock);
			server->sendClientAllIPs(conn);
			server->knownIPs->insert(msg);

			msg = "Ready for query.";
			conn->msg_send(msg);	//server response

			msg = conn->msg_recv(); // client query
			bool found = false;

			//SQLITE STUFF HERE
			std::string results = server->executeSQL(msg);
			if (results == "error") {
				conn->msg_send("Don't have it.");
			}
			else {
				conn->msg_send(results);
			}
			//ENDSQLITE STUFF HERE

		} while (msg != "");
	}

	std::string queryFromClient(std::string clientQuery) {

		//ADD DATABASE CONNECTION HERE (local database)
		std::string results = executeSQL(clientQuery);
		if (results != "error" && results != "") {
			//DO STUFF WITH RESULTS HERE!!!!!
			return results;
		}
		else {
			//if the data wasn't found return an unsuccessful query
			return "Unsuccessful Query";
		}

		//END DATABASE CONNECTION
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