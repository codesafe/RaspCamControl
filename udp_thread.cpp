
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "udp_thread.h"
#include "camera_thread.h"
#include "udpsocket.h"
#include "utils.h"

bool udp_thread::exitthread;
pthread_mutex_t udp_thread::mutex_lock;
pthread_mutex_t udp_thread::exitmutex_lock;


udp_thread::udp_thread()
{
}

udp_thread::~udp_thread()
{
}

void udp_thread::init(camerathread **threadlist)
{
	pthread_mutex_init(&mutex_lock, NULL);
	pthread_mutex_init(&exitmutex_lock, NULL);

	int err = pthread_create(&threadid, NULL, thread_fn, (void*)threadlist);

}

void* udp_thread::thread_fn(void* arg)
{
	camerathread **threadlist = (camerathread**)arg;

	// UDP Sock
	UDP_Socket udp_socket;

	if (udp_socket.init() == false)
	{
		Logger::log("UDP Socket init failed.");
		return((void*)-1);
	}

	while (true)
	{
		char buf[UDP_BUFFER] = { 0, };
		int recv = udp_socket.update(buf);

		Logger::log("Recv UDP %d", recv);

		char packet = buf[0];


		switch (packet)
		{
			case PACKET_HALFPRESS:
			case PACKET_SHOT:
			{
				for (int i = 0; i < MAX_CAMERA; i++)
				{
					if (threadlist[i] != nullptr)
						threadlist[i]->addTestPacket(buf, i);
				}
			}
			break;

			default:
				break;
		}


/*
		{
			// check EXIT
			pthread_mutex_lock(&exitmutex_lock[camnum]);

			// 스레드 끝!
			if (exitthread[camnum])
			{
				pthread_mutex_unlock(&exitmutex_lock[camnum]);
				break;
			}

			pthread_mutex_unlock(&exitmutex_lock[camnum]);
		}
*/

// 		{
// 			pthread_mutex_lock(&mutex_lock);
// 
// 			// 네트웍 업데이트 / 명령어 처리
// 			Update(camnum);
// 
// 			pthread_mutex_unlock(&mutex_lock);
// 		}
// 
		Utils::Sleep(0);
	}


	pthread_exit((void*)0);
	return((void*)0);
}

