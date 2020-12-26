
#include "predef.h"
#include "tcpsocket.h"


TCP_Socket::TCP_Socket()
{
	sock = -1;
}

TCP_Socket::~TCP_Socket()
{
}

bool TCP_Socket::init()
{
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		Logger::log("Could not create socket.");
		return false;
	}

	Logger::log("init tcp server address : %s", server_address.c_str());
	memset((void*)&server, 0x00, sizeof(server));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(server_address.c_str());
	server.sin_port = htons(SERVER_TCP_PORT);

	int flag = 1;
	int ret = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
	if (ret == -1)
	{
		Logger::log("socket Setoption. Error!");
		return false;
	}

	static int reuseFlag = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseFlag, sizeof reuseFlag) != 0) 
	{
		Logger::log("[%s] setsockopt(SO_REUSEADDR) error: ", __FUNCTION__);
		close(sock);
		return false;
	}

/*
	int curFlags = fcntl(sock, F_GETFL, 0);
	if (fcntl(sock, F_SETFL, curFlags | O_NONBLOCK) < 0)
	{
		Logger::log("Set nonblock");
		close(sock);
		return false;
	}
*/

	return true;
}

void TCP_Socket::destroy()
{
	if (sock != -1)
	{
		close(sock);
		sock = -1;
	}
}

bool TCP_Socket::connect()
{

#if 1
	int ret, err;
	fd_set set;
	FD_ZERO(&set);
	timeval tvout = { 3, 0 };  // timeout  
	FD_SET(sock, &set);

	if ((ret = ::connect(sock, (struct sockaddr*)&server, sizeof(server))) != 0)
	{
		err = errno;
		if (err != EINPROGRESS && err != EWOULDBLOCK) 
		{
			printf("connect() failed : %d\n", err);
			return false;
		}

		if (select(sock + 1, NULL, &set, NULL, &tvout) <= 0) 
		{
			printf("select/connect() failed : %d\n", errno);
			return false;
		}

		err = 0;
		socklen_t len = sizeof(err);
		if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&err, &len) < 0 || err != 0) 
		{
			printf("getsockopt() error: %d\n", err);
			return false;
		}
	  }
#else

	int err = ::connect(sock, (struct sockaddr*)&server, sizeof(server));
	if (err != 0)
	{
		Logger::log("connect failed. Error!");
		return false;
	}
#endif

	Logger::log("connected %s : %d", server_address.c_str(), SERVER_TCP_PORT);

	return true;
}

int TCP_Socket::update(char* buf)
{
	return 1;
}

int TCP_Socket::send(char* buf)
{
	return ::send(sock, buf, TCP_BUFFER, 0);
}

int TCP_Socket::recv(char* buf)
{
	return ::recv(sock, buf, TCP_BUFFER, 0);
}

int TCP_Socket::recv()
{
	return ::recv(sock, recvbuf, TCP_BUFFER, 0);
}


