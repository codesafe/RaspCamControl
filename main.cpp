

#include "predef.h"
#include "utils.h"
#include "camera_thread.h"
#include "camera_manager.h"
#include "tcpsocket.h"
#include "udpsocket.h"


/*
#include "network.h"
#include "commander.h"

#include "CameraController.h"
*/


// Aperture
string apertureString[] = { "5", "5.6", "6.3", "7.1", "8", "9", "10", "11", "13", "14",  "16", "18", "20", "22", "25", "29", "32" };
// ISO
string isoString[] = { "Auto", "100", "200", "400", "800", "1600", "3200", "6400" };
// Shutter Speed
string shutterspeedString[] = {
"bulb", "30", "25", "20", "15", "13", "10", "8", "6", "5", "4", "3.2", "2.5", "2", "1.6", "1.3", "1", "0.8", "0.6", "0.5", "0.4",
"0.3", "1/4", "1/5", "1/6", "1/8", "1/10", "1/13", "1/15", "1/20", "1/25", "1/30", "1/40", "1/50", "1/60", "1/80", "1/100", "1/125",
"1/160", "1/200", "1/250", "1/320", "1/400", "1/500", "1/640", "1/800", "1/1000", "1/1250", "1/1600", "1/2000", "1/2500", "1/3200", "1/4000" };
// Capture format
string captureformatString[] = { "Large Fine JPEG", "Large Normal JPEG", "Medium Fine JPEG", "Medium Normal JPEG", "Small Fine JPEG",
"Small Normal JPEG", "Smaller JPEG", "Tiny JPEG", "RAW + Large Fine JPEG", "RAW" };

string iso = isoString[ISO_VALUE];
string aperture = apertureString[APERTURE_VALUE];
string shutterspeed = shutterspeedString[SHUTTERSPEED_VALUE];
string captureformat = captureformatString[CAPTURE_FORMAT_VALUE];

string server_address = "";// SERVER_ADD;
string machine_name = "";
string ftp_path = "";
string camera_id = "";

TCP_Socket tcp_socket;
UDP_Socket udp_socket;


void LoadConfig()
{
	Logger::log("Load Config.");

	FILE* fp = fopen("config.txt", "rt");
	if (fp != NULL)
	{
		// 머신 이름
		char name[32] = { 0, };
		fgets(name, sizeof(name), fp);
		Utils::clearString(name);
		machine_name = name;
		Logger::log("machine name : %s", machine_name.c_str());

		// 서버 주소
		char address[32] = { 0, };
		fgets(address, sizeof(address), fp);
		Utils::clearString(address);
		server_address = string(address);

		Logger::log("server address : %s", server_address.c_str());
		fclose(fp);
	}
	else
		Logger::log("Not found config.");
}


bool initcamera()
{
	std::vector<camerathread*> threadlist;
	camera_manager::getInstance()->enumCameraList();
	int len = camera_manager::getInstance()->getEnumCameraNum();
	for (int i = 0; i < len; i++)
	{
		camerathread* thread = new camerathread();
		thread->init(i);
		//Utils::Sleep(1.0f);
		threadlist.push_back(thread);
	}

	// Send camera num & machine name
	char buf[TCP_BUFFER] = { 0, };
	buf[0] = (char)len;
	strcpy(buf+1, machine_name.c_str());
	tcp_socket.send(buf);

	return true;
}

int main(void)
{
	LoadConfig();

	CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res != CURLcode::CURLE_OK)
	{
		Logger::log("CURL init error : %d", res);
		return res;
	}

	// TODO. Connect to server and receive info
	tcp_socket.init();
	if (tcp_socket.connect() == false)
	{
		Logger::log("TCP connect error : %s", server_address.c_str());
		return -1;
	}

/*
	int recv = tcp_socket.recv();
	if (recv < 1)
	{
		Logger::log("TCP recv error : %s", server_address.c_str());
		return -1;
	}

*/
	// UDP Sock
	if (udp_socket.init() == false)
	{
		Logger::log("UDP Socket init failed.");
		return -1;
	}

	// 카메라 초기화
	initcamera();

	// 	CameraThread* thread = new CameraThread();
	// 	thread->Start(0);
	// 	threadlist.push_back(thread);

	while (true)
	{
		Utils::Sleep(10000);
		/*
				// for test
				int i = getch();
				printf("Input = %d\n", i);
				if (i == '0')
				{
					for (int i = 0; i < threadlist.size(); i++)
					{
						threadlist[i]->addTestPacket(PACKET_TRY_CONNECT, i);
					}
				}
				else if (i == '1')
				{
					for (int i = 0; i < threadlist.size(); i++)
					{
						threadlist[i]->addTestPacket(PACKET_HALFPRESS, i);
					}
				}
				else if (i == '2')
				{
					for (int i = 0; i < threadlist.size(); i++)
					{
						threadlist[i]->addTestPacket(PACKET_SHOT, i);
					}
				}
				else if (i == '3')
				{
					for (int i = 0; i < threadlist.size(); i++)
					{
						threadlist[i]->addTestPacket(PACKET_FORCE_UPLOAD, i);
					}
				}
		*/
	}

	curl_global_cleanup();

	return 0;
}