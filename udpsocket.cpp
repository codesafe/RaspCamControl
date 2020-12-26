#include "predef.h"
#include "udpsocket.h"

UDP_Socket::UDP_Socket()
{
	sock = -1;
}

UDP_Socket::~UDP_Socket()
{
	if (sock != -1)
		close(sock);
}

bool UDP_Socket::init(int port)
{
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		Logger::log("UDP Sock failed.");
		return false;
	}

	memset(&servAddr, 0, sizeof(servAddr));

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(port);

	static int reuseFlag = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseFlag, sizeof reuseFlag) != 0)
	{
		Logger::log("[%s] UDP setsockopt(SO_REUSEADDR) error: ", __FUNCTION__);
		close(sock);
		return false;
	}

	Logger::log("Init UDP Port : %d", port);

	if (bind(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) 
	{
		Logger::log("UDP_Socket Bind failed. %s", strerror(errno));
		return false;
	}

	return true;
}

int UDP_Socket::update(char *buf)
{
	int client_addr_size = sizeof(clntAddr);
	return recvfrom(sock, buf, UDP_BUFFER, 0, (struct sockaddr*) &clntAddr, (socklen_t *)&client_addr_size);
}

void UDP_Socket::send(char* buf, int bufsize)
{
	int sentsize = sendto(sock, buf, bufsize, 0, (struct sockaddr*) & servAddr, sizeof(servAddr));

	if (sentsize != bufsize)
	{
		Logger::log("UDP_Socket Sendto failed.");
		return;
	}
}