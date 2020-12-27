
#ifndef _CAMERA_THREAD_
#define _CAMERA_THREAD_

#include "predef.h"

class camcontrol;
class UDP_Socket;

struct ThreadPassInfo
{
	int cameranum;
	int udp_port;
};

class camerathread
{
public :
	camerathread();
	~camerathread();

	void init(int camnumber, char macninenum);
	static void* thread_fn(void* arg);

	void addTestPacket(char *packet, int camnum);
	bool getSendPacket(int camnumber, char* buf);
	void wakeup(int camnumber);

private:

	ThreadPassInfo info;

	//static void Update(int camnum);
	static int parsePacket(int camnum, char* buf);
	static bool StartUpload(int camnum);
	static size_t read_callback(void* ptr, size_t size, size_t nmemb, void* userp);
	static void addSendPacket(int camnum, char* buf);
	static string GetError(int errorcode);

	pthread_t		threadid;

	static camcontrol* cameras[MAX_CAMERA];
	static bool exitthread[MAX_CAMERA];

	static pthread_mutex_t mutex_lock[MAX_CAMERA];
	static pthread_mutex_t exitmutex_lock[MAX_CAMERA];

	static int upload_progress[MAX_CAMERA];
	static std::deque<char*> packetbuffer[MAX_CAMERA];
	static char recvBuffer[MAX_CAMERA][UDP_BUFFER];

	static WriteThis upload[MAX_CAMERA];
	static UDP_Socket udpsocket[MAX_CAMERA];
	static float delaytime[MAX_CAMERA];
};



#endif