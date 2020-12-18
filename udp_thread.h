
#ifndef _UDP_THREAD_
#define _UDP_THREAD_

#include "predef.h"

class camerathread;

class udp_thread
{
public:
	udp_thread();
	~udp_thread();

	void init(camerathread **threadlist);
	static void* thread_fn(void* arg);

private:
	pthread_t		threadid;

	static bool exitthread;
	static pthread_mutex_t mutex_lock;
	static pthread_mutex_t exitmutex_lock;



};



#endif