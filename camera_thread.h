
#ifndef _CAMERA_THREAD_
#define _CAMERA_THREAD_

#include "predef.h"

class camcontrol;

class camerathread
{
public :
	camerathread();
	~camerathread();

	void init(int camnumber);
	static void* thread_fn(void* arg);

	void addTestPacket(char *packet, int camnum);
	bool getSendPacket(int camnumber, char* buf);
	void wakeup(int camnumber);

private:

	//static void Update(int camnum);
	static int parsePacket(int camnum);
	static bool StartUpload(int camnum);
	static size_t read_callback(void* ptr, size_t size, size_t nmemb, void* userp);
	static void addSendPacket(char* buf);

	pthread_t		threadid;

	static camcontrol* cameras[MAX_CAMERA];

	static CAMERA_STATE camera_state[MAX_CAMERA];
	static bool exitthread[MAX_CAMERA];

	static pthread_mutex_t mutex_lock[MAX_CAMERA];
	static pthread_mutex_t exitmutex_lock[MAX_CAMERA];

	static int upload_progress[MAX_CAMERA];
	static std::deque<char*> packetbuffer[MAX_CAMERA];
	static char recvBuffer[MAX_CAMERA][UDP_BUFFER];

	static pthread_cond_t cond[MAX_CAMERA];
	static pthread_mutex_t mutex[MAX_CAMERA];

	static WriteThis upload[MAX_CAMERA];

};



#endif