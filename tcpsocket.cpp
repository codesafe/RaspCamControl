
#include "predef.h"
#include "tcpsocket.h"


TCP_Socket::TCP_Socket()
{

}
TCP_Socket::~TCP_Socket()
{

}

void TCP_Socket::init()
{
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		Logger::log("Could not create socket.");
		return ;
	}
	Logger::log("init tcp server address : %s", server_address.c_str());

	memset((void*)&server, 0x00, sizeof(server));
	server.sin_addr.s_addr = inet_addr(server_address.c_str());
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_TCP_PORT);

	int flag = 1;
	int ret = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
	if (ret == -1)
	{
		Logger::log("socket Setoption. Error!");
		return;
	}
}

bool TCP_Socket::connect()
{
	int err = ::connect(sock, (struct sockaddr*)&server, sizeof(server));
	if (err < 0)
	{
		Logger::log("connect failed. Error!");
		return false;
	}

	Logger::log("connected !");

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


