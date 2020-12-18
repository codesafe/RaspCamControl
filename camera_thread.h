
#ifndef _CAMERA_THREAD_
#define _CAMERA_THREAD_

#include "predef.h"

class camcontrol;

class camerathread
{
public :
	camerathread();
	~camerathread();

	void init(int camnum);
	static void* thread_fn(void* arg);

	void addTestPacket(char *packet, int camnum);

private:
	static void Update(int camnum);
	static int parsePacket(int camnum, char* buf);
	static bool StartUpload(int camnum);
	static size_t read_callback(void* ptr, size_t size, size_t nmemb, void* userp);

	pthread_t		threadid;

	static camcontrol* cameras[MAX_CAMERA];

	static CAMERA_STATE camera_state[MAX_CAMERA];
	static bool exitthread[MAX_CAMERA];

	static pthread_mutex_t mutex_lock[MAX_CAMERA];
	static pthread_mutex_t exitmutex_lock[MAX_CAMERA];

	static int upload_progress[MAX_CAMERA];

};



#endif