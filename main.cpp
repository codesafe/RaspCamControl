

#include "predef.h"
#include "utils.h"
#include "camera_thread.h"
#include "camera_manager.h"
#include "tcpsocket.h"
#include "udpsocket.h"
#include "udp_thread.h"

//==============================================================================================================================

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

//==============================================================================================================================


string iso = isoString[ISO_VALUE];
string aperture = apertureString[APERTURE_VALUE];
string shutterspeed = shutterspeedString[SHUTTERSPEED_VALUE];
string captureformat = captureformatString[CAPTURE_FORMAT_VALUE];

bool recieved_serveraddress = false;
string server_address = "";// SERVER_ADD;
string machine_name = "";
string capturefile_ext = "jpg";
string ftp_path = "";
string ftp_id = "";
string ftp_passwd = "";
string camera_id = "";

TCP_Socket tcp_socket;
//UDP_Socket udp_socket;
camerathread* threadlist[MAX_CAMERA] = { nullptr, };


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

		// ftp id
		char id[32] = { 0, };
		fgets(id, sizeof(id), fp);
		Utils::clearString(id);
		ftp_id = string(id);
		Logger::log("ftp id : %s", ftp_id.c_str());

		// ftp pass
		char pass[32] = { 0, };
		fgets(pass, sizeof(pass), fp);
		Utils::clearString(pass);
		ftp_passwd = string(pass);
		Logger::log("ftp passwd: %s", ftp_passwd.c_str());

		fclose(fp);
	}
	else
		Logger::log("Not found config.");
}


bool initcamera()
{
	// 연결된 카메라 검사
	camera_manager::getInstance()->enumCameraList();
	int len = camera_manager::getInstance()->getEnumCameraNum();

	// 카메라 댓수와 머신 이름을 전송 
	char buf[TCP_BUFFER] = { 0, };
	buf[0] = (char)len;
	strcpy(buf+1, machine_name.c_str());
	tcp_socket.send(buf);

	// 할당된 카메라 번호 수신 --> UDP Port 결정
	char recvbuf[TCP_BUFFER] = { 0, };
	int nrecv = tcp_socket.recv(recvbuf);
	char macninenum = recvbuf[0];

	for (int i = 0; i < len; i++)
	{
		camerathread* thread = new camerathread();
		thread->init(i, macninenum);
		Utils::Sleep(0.5f);
		//threadlist.push_back(thread);
		threadlist[i] = thread;
	}


	return true;
}

void RecieveServerInfo()
{
	int sock;
	struct sockaddr_in broadcastAddr;
	unsigned short broadcastPort;
	char recvString[UDP_BUFFER] = { 0, };
	int recvStringLen;

	broadcastPort = SERVER_UDP_BROADCASTPORT;

	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		Logger::log("socket() failed");
		return;
	}

 	memset(&broadcastAddr, 0, sizeof(broadcastAddr));
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	broadcastAddr.sin_port = htons(broadcastPort);


	static int reuseFlag = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseFlag, sizeof reuseFlag) != 0)
	{
		Logger::log("[%s] RecieveServerInfo setsockopt(SO_REUSEADDR) error: ", __FUNCTION__);
		close(sock);
		return;
	}

	if (bind(sock, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) < 0)
	{
		Logger::log("RecieveServerInfo bind() failed");
		return;
	}

	if ((recvStringLen = recvfrom(sock, recvString, UDP_BUFFER, 0, NULL, 0)) < 0)
	{
		Logger::log("RecieveServerInfo recvfrom() failed");
		return;
	}
	//recvString[recvStringLen] = '\0';

	char vbuff[UDP_BUFFER] = { 0, };
	sprintf(vbuff, "%d.%d.%d.%d", (unsigned char)recvString[1], (unsigned char)recvString[2],
		(unsigned char)recvString[3], (unsigned char)recvString[4]);

	//Logger::log("Received: %d : %s\n", recvString[0], vbuff);

	if (recvString[0] == UDP_BROADCAST_PACKET)
	{
 		server_address = string(vbuff);
 		Logger::log("Recv server address : %s", server_address.c_str());
	}

	close(sock);
}


void InitSystem()
{
	LoadConfig();
	RecieveServerInfo();
}

int main(void)
{
	InitSystem();

	CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res != CURLcode::CURLE_OK)
	{
		Logger::log("CURL init error : %d", res);
		return res;
	}

	while (true)
	{
		// TODO. Connect to server and receive info
		if (tcp_socket.init() == false)
			return -1;

		bool connected = tcp_socket.connect();

		if(connected == false)
		{
			Utils::Sleep(3.0f);
			Logger::log("retry connect to : %s", server_address.c_str());
			tcp_socket.destroy();
		}
		else
			break;
	}

	// 카메라 초기화
	initcamera();

// 안쓰는거로
// 	udp_thread* udpthread = new udp_thread();
// 	udpthread->init(threadlist);

	char tcpbuffer[TCP_BUFFER] = { 0, };

	while (true)
	{
		for (int i = 0; i < MAX_CAMERA; i++)
		{
			if(threadlist[i] == nullptr) continue;
			if (threadlist[i]->getSendPacket(i, tcpbuffer) == true)
				tcp_socket.send(tcpbuffer);
		}

		Utils::Sleep(0.1f);
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