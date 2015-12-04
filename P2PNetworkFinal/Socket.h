#ifndef _SOCKET_H_
#define _SOCKET_H_
#ifdef WIN32 // Windows related includes
#include <windows.h>
#include <winsock.h>
#define SOCKET_TYPE	SOCKET
#else		// Linux related includes
#include <sys/types.h>          
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#define SOCKET_ERROR	-1
#define SOCKET_TYPE	int
#endif
// Common includes for Windows and Linux
#include <iostream>
#include <stdexcept>
#include <string>

/**
*	Class Socket for encapsulating the winsock functionalities
*	This is a work in progress.  It currently only supports TCP connections.
*	Compatible across Windows and Linux.
*	@author	Ed Walker
*/
class Socket
{
private:
	SOCKET_TYPE sock; //  system level socket

	struct sockaddr_in recv_addr; // Address of peer 
	int *ref_count; // reference count - number of objects holding the same SOCKET

					/**
					* Constructor for initializing with an already created socket
					*/
	Socket(SOCKET_TYPE &sock, struct sockaddr_in &recv_addr)
	{
		this->sock = sock;
		memcpy(&(this->recv_addr), &recv_addr, sizeof(recv_addr));
		ref_count = new int(1);
	}

public:
	/**
	*	Static function for initializing the winsock library
	*	This MUST  be called once in the main function before any of the
	*	socket functions are invoked.
	*/
	static bool Init()
	{
#ifdef WIN32 // Windows ONLY
		WORD sockVersion;
		WSADATA wsaData;

		// We'd like Winsock version 2.0
		sockVersion = MAKEWORD(2, 0);

		// We begin by initializing Winsock
		int error = WSAStartup(sockVersion, &wsaData);
		if (error != 0) {
			std::cerr << "WSAStartup failed\n";
			return false;
		}

		/* Check for correct version */
		if (LOBYTE(wsaData.wVersion) != 2 ||
			HIBYTE(wsaData.wVersion) != 0)
		{
			/* incorrect WinSock version */
			WSACleanup();
			std::cerr << "Incorrect version\n";
			return false;
		}
#endif
		return true;
	}

	/**
	*	Static function for cleaning up after the winsock library.
	*	This should be called at the end of the main function.
	*/
	static void Cleanup()
	{
#ifdef WIN32 // Windows ONLY
		WSACleanup();
#endif
	}

	/**
	*	Constructor for creating either a TCP or UDP socket
	*	@param proto	A string which is either "tcp" or "udp"
	*/
	Socket(std::string proto)
	{
		if (proto == "tcp") {
			sock = socket(AF_INET, SOCK_STREAM, 0);
		}
		else if (proto == "udp")
			sock = socket(AF_INET, SOCK_DGRAM, 0);
		else
			throw std::invalid_argument("Invalid socket protocol type");
		ref_count = new int(1);
	}

	/**
	*	Copy constructor
	*	This constructor will increment the reference count to the socket.
	*/
	Socket(const Socket &copy)
	{
		sock = copy.sock;
		memcpy(&recv_addr, &(copy.recv_addr), sizeof(recv_addr));
		ref_count = copy.ref_count; // we want all copies to reference the same reference counter
		++(*ref_count);
	}

	/**
	*	Assignment operator
	*	The assignment operator will increment the reference count to the socket
	*/
	Socket &operator=(const Socket &copy)
	{
		sock = copy.sock;
		memcpy(&recv_addr, &(copy.recv_addr), sizeof(recv_addr));
		ref_count = copy.ref_count; // we want all copies to reference the same reference counter
		++(*ref_count);
		return *this;
	}

	/**
	*	Destructor
	*	This will decrement the reference counter, and
	*	close the socket if reference count == 0
	*/
	virtual ~Socket()
	{
		--(*ref_count);
		// Multiple Socket objects may be holding this socket.
		// Therefore it is important to check who else owns it 
		// before we attempt to close it.
		if (*ref_count == 0) {
#ifdef WIN32 // Windows
			closesocket(sock);
#else		// Linux
			close(sock);
#endif
		}
	}

	/**
	*	Gets the IP address of a host name.
	*	@param	hostname		[IN] Hostname for querying for IP address
	*	@param	official_name	[OUT] Offical hostname from DNS
	*	@return The IP address if the hostname has one or "" otherwise
	*/
	static std::string getaddr(std::string &hostname, std::string &offical_name) {
		struct hostent * remotehost = gethostbyname(hostname.c_str());
		if (remotehost != NULL) {
			offical_name = remotehost->h_name;
			struct in_addr **addr;
			addr = (struct in_addr **) remotehost->h_addr_list; // array of pointers to in_addr
			return std::string(inet_ntoa(*addr[0])); // just get the first address
		}
		return "";
	}

	/**
	*	Connect socket to address and port number
	*
	*	This should be called by the client only
	*	@param addr	IPv4 address
	*	@param port	Port number
	*/
	bool sock_connect(std::string addr, int port)
	{
		struct sockaddr_in sin;

		memset(&sin, 0, sizeof sin);

		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = inet_addr(addr.c_str());	// address to connect too
		sin.sin_port = htons(port);					// port to connect too

		return (connect(sock, (sockaddr *)&sin, sizeof(sin)) != SOCKET_ERROR);
	}

	/**
	*	Bind socket to the local address and a port number
	*
	*	@param addr	IPv4 address or "" for all interfaces
	*	@param port Port number
	*/
	bool sock_bind(std::string addr, int port)
	{
		struct sockaddr_in sin;

		memset(&sin, 0, sizeof sin);

		sin.sin_family = AF_INET;
		if (addr == "")
			sin.sin_addr.s_addr = INADDR_ANY;
		else
			sin.sin_addr.s_addr = inet_addr(addr.c_str());
		sin.sin_port = htons(port);

		return (bind(sock, (const sockaddr *)&sin, sizeof sin) != SOCKET_ERROR);
	}

	/**
	*	Configure socket to listen for connections from clients
	*
	*	@param conn	Number of pending connections in queue
	*/
	bool sock_listen(int conn)
	{
		return (listen(sock, conn) != SOCKET_ERROR);
	}

	/**
	*	Wait for a connection
	*
	*	@return Connected socket. Use this to communicate with client.
	*/
	Socket *sock_accept()
	{
#ifdef WIN32	// Windows
		int length;
#else			// Linux
		socklen_t length;
#endif
		length = sizeof(recv_addr);
		SOCKET_TYPE client = accept(sock, (sockaddr *)&recv_addr, &length);
		return new Socket(client, recv_addr);
	}


	/**
	*	Close SOCKET
	*/
	void sock_close()
	{
#ifdef WIN32	// Windows
		closesocket(sock);
#else			// Linux
		close(sock);
#endif
	}

	/**
	*	Send a string to connected peer
	*
	*	@param msg	String to send down the socket
	*/
	bool msg_send(const std::string &msg)
	{
		int sent = 0;
		do {
			int rc = send(sock, msg.c_str(), msg.length(), 0);
			if (rc <= 0)
				break;
			sent += rc;
		} while (sent < msg.length());
		return (sent == msg.length());
	}

	/**
	*	Receive a short string from connected peer
	*
	*	@return Short string (up to 1024 bytes) read from the socket
	*/
	std::string msg_recv()
	{
		std::string rc_string;

		char buf[2048];
		int rc = recv(sock, buf, 2047, 0);
		if (rc >= 0) {
			buf[rc] = '\0'; // NULL terminate the message
			rc_string = buf;
		}
		return rc_string;
	}

	/**
	*	Receive method for string of size bytes, or
	*	until socket connection is closed.
	*
	*	@param size Size of message that must be received before this method
	*	@return	String not longer than size bytes
	*/
	std::string msg_recvall(int size)
	{
		std::string rc_string;
		char buf[2048];

		do
		{
			// read in chunks of 2047 bytes
			int rc = recv(sock, buf, 2047, 0);
			if (rc >= 0) {
				size -= rc;
				buf[rc] = '\0'; // make sure we terminate with '\0'
				rc_string += buf; // append to return string
			}
			else
				break; // socket connection is closed!
		} while (size > 0); // keep on reading until we read all size bytes
		return rc_string;
	}

	//returns the IP address of the current computer in the form of a string
	#pragma comment(lib, "wsock32.lib")
		std::string getComputerIP()
		{
			WORD wVersionRequested;
			WSADATA wsaData;
			char name[255];
			PHOSTENT hostinfo;
			wVersionRequested = MAKEWORD(1, 1);
			char *ip;

			if (WSAStartup(wVersionRequested, &wsaData) == 0)
				if (gethostname(name, sizeof(name)) == 0)
				{
					//printf("Host name: %s\n", name);

					if ((hostinfo = gethostbyname(name)) != NULL)
					{
						int nCount = 0;
						while (hostinfo->h_addr_list[nCount])
						{
							ip = inet_ntoa(*(
							struct in_addr *)hostinfo->h_addr_list[nCount]);

							//printf("IP #%d: %s\n", ++nCount, ip);
							return std::string(ip);
						}
					}
				}
		}


};
#endif