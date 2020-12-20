
/*
gphoto2 --force-overwrite --set-config imageformat=3 --set-config iso=0
--set-config eosremoterelease=5 --wait-event-and-download=2s --set-config eosremoterelease=4 --filename image2.jpg


gphoto2 --set-config eosremoterelease=5 --wait-event-and-download=FILEADDED --set-config eosremoterelease=4 --filename image2.jpg


이런식으로 Immediate 사용가능
Choice: 0 None
Choice: 1 Press Half
Choice: 2 Press Full
Choice: 3 Release Half
Choice: 4 Release Full
Choice: 5 Immediate
Choice: 6 Press 1
Choice: 7 Press 2
Choice: 8 Press 3
Choice: 9 Release 1
Choice: 10 Release 2
Choice: 11 Release 3
*/

#include <pthread.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>


#include "camera_thread.h"
#include "camera_manager.h"
#include "camcontrol.h"
#include "utils.h"



/*
#define 	GP_ERROR_CORRUPTED_DATA   -102
Corrupted data received.More...
#define 	GP_ERROR_FILE_EXISTS   -103
File already exists.More...
#define 	GP_ERROR_MODEL_NOT_FOUND   -105
Specified camera model was not found.More...
#define 	GP_ERROR_DIRECTORY_NOT_FOUND   -107
Specified directory was not found.More...
#define 	GP_ERROR_FILE_NOT_FOUND   -108
Specified file was not found.More...
#define 	GP_ERROR_DIRECTORY_EXISTS   -109
Specified directory already exists.More...
#define 	GP_ERROR_CAMERA_BUSY   -110
The camera is already busy.More...
#define 	GP_ERROR_PATH_NOT_ABSOLUTE   -111
Path is not absolute.More...
#define 	GP_ERROR_CANCEL   -112
Cancellation successful.More...
#define 	GP_ERROR_CAMERA_ERROR   -113
Unspecified camera error.More...
#define 	GP_ERROR_OS_FAILURE   -114
Unspecified failure of the operating system.More...
#define 	GP_ERROR_NO_SPACE   -115
Not enough space.More...
*/

string GetError(int errorcode)
{
	switch (errorcode)
	{
	case GP_ERROR_CORRUPTED_DATA:
		return string("GP_ERROR_CORRUPTED_DATA");

	case GP_ERROR_FILE_EXISTS:
		return string("GP_ERROR_FILE_EXISTS");
	case GP_ERROR_MODEL_NOT_FOUND:
		return string("GP_ERROR_MODEL_NOT_FOUND");
	case GP_ERROR_DIRECTORY_NOT_FOUND:
		return string("GP_ERROR_DIRECTORY_NOT_FOUND");
	case GP_ERROR_FILE_NOT_FOUND:
		return string("GP_ERROR_FILE_NOT_FOUND");
	case GP_ERROR_DIRECTORY_EXISTS:
		return string("GP_ERROR_DIRECTORY_EXISTS");
	case GP_ERROR_CAMERA_BUSY:
		return string("GP_ERROR_CAMERA_BUSY");
	case GP_ERROR_PATH_NOT_ABSOLUTE:
		return string("GP_ERROR_PATH_NOT_ABSOLUTE");
	case GP_ERROR_CANCEL:
		return string("GP_ERROR_CANCEL");
	case GP_ERROR_CAMERA_ERROR:
		return string("GP_ERROR_CAMERA_ERROR");
	case GP_ERROR_OS_FAILURE:
		return string("GP_ERROR_OS_FAILURE");
	case GP_ERROR_NO_SPACE:
		return string("GP_ERROR_NO_SPACE");
	}

	string errorstr = gp_port_result_as_string(errorcode);
	return errorstr;
}

///////////////////////////////////////////////////////////////////////////////////////////////


camcontrol* camerathread::cameras[MAX_CAMERA];
CAMERA_STATE camerathread::camera_state[MAX_CAMERA];

bool camerathread::exitthread[MAX_CAMERA];
pthread_mutex_t camerathread::mutex_lock[MAX_CAMERA];
pthread_mutex_t camerathread::exitmutex_lock[MAX_CAMERA];

pthread_cond_t camerathread::cond[MAX_CAMERA];
pthread_mutex_t camerathread::mutex[MAX_CAMERA];

int camerathread::upload_progress[MAX_CAMERA];
std::deque<char*> camerathread::packetbuffer[MAX_CAMERA];
char camerathread::recvBuffer[MAX_CAMERA][UDP_BUFFER];
WriteThis camerathread::upload[MAX_CAMERA];

thread_local unsigned int cameraNum;

camerathread::camerathread()
{
}

camerathread::~camerathread()
{
}

void camerathread::init(int camnumber)
{
	camera_state[camnumber] = CAMERA_STATE::STATE_STARTCONNECT;

	pthread_mutex_init(&mutex_lock[camnumber], NULL);
	pthread_mutex_init(&exitmutex_lock[camnumber], NULL);

	//int err = 
	pthread_create(&threadid, NULL, thread_fn, (void*)camnumber);
}


void* camerathread::thread_fn(void* arg)
{
	cameraNum = (unsigned int)arg;

	camcontrol* camera = camera_manager::getInstance()->getCamera(cameraNum);
	if (camera == NULL)
	{
		Logger::log(-1, "Thread : Camera Create fail");
		return((void*)0);
	}
	else
	{
		Logger::log(-1, "Thread : Camera Create success %d", cameraNum);
	}
 	cameras[cameraNum] = camera;

	// main thread loop : Wakeup 시그널이 올때까지 영원히 대기
	while (true)
	{
		Logger::log(cameraNum, "Enter Wait State Camera");
		pthread_cond_wait(&cond[cameraNum], &mutex[cameraNum]);
		parsePacket(cameraNum);
	}

	pthread_exit((void*)0);
	return((void*)0);
}

#if 0
void camerathread::Update(int camnum)
{
	switch (camera_state[camnum])
	{
		case CAMERA_STATE::STATE_NONE:
			break;

		case CAMERA_STATE::STATE_CONNECT_ERROR:
			Logger::log(camnum, "STATE STATE_CONNECT_ERROR. Retry Connect.");
			Utils::Sleep(2);
			camera_state[camnum] = CAMERA_STATE::STATE_STARTCONNECT;
			return;

		case CAMERA_STATE::STATE_STARTCONNECT:
			{
				Logger::log(camnum, "Start Connect to Server.");
// 				bool connected = tcpsocket[camnum].connect();
// 				if (connected)
// 					camera_state[camnum] = CAMERA_STATE::STATE_CONNECTION;
// 				else
// 					camera_state[camnum] = CAMERA_STATE::STATE_CONNECT_ERROR;

				camera_state[camnum] = CAMERA_STATE::STATE_READY;
			}
			break;

		case CAMERA_STATE::STATE_CONNECTION:
			{
// 				char buf[TCP_BUFFER];
// 				int recvbyte = tcpsocket[camnum].recv(buf);
// 				if (recvbyte > 0)
// 				{
// 					camera_serverid[camnum] = buf[0];
// 					udpsocket[camnum].init(camera_serverid[camnum]);
// 					camera_state[camnum] = CAMERA_STATE::STATE_READY;
// 					Logger::log(camnum, "--------------------------------------------------------");
// 					Logger::log(camnum, "Connected! Camera Server ID : %d", camera_serverid[camnum]);
// 					Logger::log(camnum, "--------------------------------------------------------");
// 
// 				}
			}
			break;

		case CAMERA_STATE::STATE_READY:
			{
// 				char buf[UDP_BUFFER];
// 				int ret = udpsocket[camnum].update(buf);
// 				if (ret > 0)
// 					parsePacket(camnum, buf);
// 				else
// 					Logger::log(camnum, "Recv UDP error");
			}
			break;

		case CAMERA_STATE::STATE_FOCUSING:
		case CAMERA_STATE::STATE_SHOT:
			break;

		case CAMERA_STATE::STATE_UPLOAD:
		{
			StartUpload(camnum);
			return;
		}
		break;

		case CAMERA_STATE::STATE_UPLOADING:
		{
// 			if (upload_progress[camnum] == 10)
// 			{
// 				char data[10];
// 				network[camnum].write(PACKET_UPLOAD_DONE, data, 10);
// 				camera_state[camnum] = CAMERA_STATE::STATE_READY;
// 			}
// 			return;
		}
		break;
	}

	//Logger::log(0,"Camera %d State : %d", camnum, camera_state[camnum]);
	// network update / parse packet / 패킷있으면 commander에 추가
	//network[camnum].update();
	// 쌓여 있는 커맨드가 있으면 여기에서 처리한다.
	//UpdateCommand(camnum);


}
#endif


int camerathread::parsePacket(int camnum)
{
	char* buf = recvBuffer[camnum];
	char packet = buf[0];
	int ret = 0;
	std::string  date = Utils::getCurrentDateTime();

	switch (packet)
	{
		case PACKET_TRY_CONNECT:
			break;

		case PACKET_HALFPRESS:
		{

			char data[TCP_BUFFER] = { 0, };
			data[0] = PACKET_AUTOFOCUS_RESULT;

/*
			if (cameras[camnum]->is_halfpressed())
			{
				ret = cameras[camnum]->set_settings_value("eosremoterelease", "Release Full");
				if (ret < GP_OK)
				{
					printf("ERR eosremoterelease Release Full : %d : %d\n", ret, camnum);
					return ret;
				}
				printf("End Release 1 : %d : %d\n", ret, camnum);
			}
*/
			unsigned char i = buf[1];	// iso
			unsigned char s = buf[2];	// shutterspeed
			unsigned char a = buf[3];	// aperture
			unsigned char f = buf[4];	// format

			Logger::log(camnum, "PACKET_HALFPRESS %d : %d : %d : %d", i, s, a, f);

			cameras[camnum]->set_essential_param(CAMERA_PARAM::ISO, isoString[i]);
			cameras[camnum]->set_essential_param(CAMERA_PARAM::SHUTTERSPEED, shutterspeedString[s]);
			cameras[camnum]->set_essential_param(CAMERA_PARAM::APERTURE, apertureString[a]);
			cameras[camnum]->set_essential_param(CAMERA_PARAM::CAPTURE_FORMAT, captureformatString[f]);

			ret = cameras[camnum]->apply_essential_param_param(camnum);
			if (ret < GP_OK)
			{
				string errorstr = GetError(ret);
				Logger::log(camnum, "ERR apply_essential_param_param : %s(%d): %d\n", errorstr.c_str(), ret, camnum);

				data[1] = (char)camnum;
				data[2] = RESPONSE_FAIL;
				addSendPacket(data);
				return ret;
			}

			// false로 하면 auto focus 안먹음
			ret = cameras[camnum]->apply_autofocus(camnum, true);
			if (ret < GP_OK)
			{
				string errorstr = GetError(ret);
				Logger::log(camnum, "ERR apply_autofocus (False) : %s(%d): %d\n", errorstr.c_str(), ret, camnum);

				data[1] = (char)camnum;
				data[2] = RESPONSE_FAIL;
				addSendPacket(data);
				return ret;
			}

			// 포커스 
			ret = cameras[camnum]->set_settings_value("eosremoterelease", "Press Half");
			if (ret < GP_OK)
			{
				string errorstr = GetError(ret);
				Logger::log(camnum, "ERR eosremoterelease Press Half : %s(%d): %d\n", errorstr.c_str(), ret, camnum);

				data[1] = (char)camnum;
				data[2] = RESPONSE_FAIL;
				addSendPacket(data);
				return ret;
			}
			else
				Logger::log(camnum, "Press Half : %d : %d\n", ret, camnum);

			ret = cameras[camnum]->set_settings_value("eosremoterelease", "Release Full");
			if (ret < GP_OK)
			{
				string errorstr = GetError(ret);
				Logger::log(camnum, "ERR eosremoterelease Release Full : %s(%d): %d\n", errorstr.c_str(), ret, camnum);

				data[1] = (char)camnum;
				data[2] = RESPONSE_FAIL;
				addSendPacket(data);

				return ret;
			}
/*
			ret = cameras[camnum]->apply_autofocus(camnum, false);
			if (ret < GP_OK)
			{
				string errorstr = GetError(ret);
				Logger::log(camnum, "ERR apply_autofocus (False) : %s(%d): %d\n", errorstr.c_str(), ret, camnum);

				data[1] = (char)camnum;
				data[2] = RESPONSE_FAIL;
				return ret;
			}*/

			data[1] = (char)camnum;
			data[2] = RESPONSE_OK;
			addSendPacket(data);
			Logger::log(camnum, "PACKET_HALFPRESS --> RESPONSE_OK");
		}
		break;

		case PACKET_SHOT:
		{
			Logger::log("---> Shot : %d : %s", camnum, date.c_str());

			// ftp path 읽어야함
			ftp_path = (char*)buf+1;

			// 찍어
			string name = Utils::format_string("%s-%d.%s", machine_name.c_str(), camnum, capturefile_ext.c_str());
			
			int ret = cameras[camnum]->capture3(name.c_str());

			if (ret < GP_OK)
			{
				camera_state[camnum] = CAMERA_STATE::STATE_READY;
				Logger::log(camnum, "Shot Error : (%d)", ret);
			}
			else
			{
				std::string  date = Utils::getCurrentDateTime();
				Utils::Sleep(1);
				//int ret = cameras[camnum]->downloadimage(name.c_str());
				camera_state[camnum] = CAMERA_STATE::STATE_UPLOAD;

				StartUpload(camnum);
			}
		}
		break;

/*
		case PACKET_ISO:
		{
			//int value = (int&)*(it->data);
			//string value = (char*)buf[1];
			char v = buf[1];
			iso = isoString[v];
			cameras[camnum]->set_essential_param(CAMERA_PARAM::ISO, iso);
		}
		break;

		case PACKET_APERTURE:
		{
			//int value = (int&)*(it->data);
			char v = buf[1];
			aperture = apertureString[v];
			cameras[camnum]->set_essential_param(CAMERA_PARAM::APERTURE, apertureString[v]);
		}
		break;

		case PACKET_SHUTTERSPEED:
		{
			//int value = (int&)*(it->data);
			char v = buf[1];
			shutterspeed = shutterspeedString[v];
			cameras[camnum]->set_essential_param(CAMERA_PARAM::SHUTTERSPEED, shutterspeed);
		}
		break;
*/

		case PACKET_FORCE_UPLOAD:
			//camera_state[camnum] = CAMERA_STATE::STATE_UPLOAD;
			break;
	}

	return ret;
}




void camerathread::addTestPacket(char *packet, int camnum)
{
	memcpy(recvBuffer[camnum], packet, UDP_BUFFER);

//	parsePacket(camnum, packet);

/*
	pthread_mutex_lock(&mutex_lock[camnum]);

	Commander* commander = network[camnum].getcommander();
	char data[10] = { 1, };
	commander->addcommand(packet, data, 10);

	pthread_mutex_unlock(&mutex_lock[camnum]);
*/

}

bool camerathread::StartUpload(int camnum)
{
	camera_state[camnum] = CAMERA_STATE::STATE_UPLOADING;
	upload_progress[camnum] = 0;

	CURL* curl;
	CURLcode res;
	//struct WriteThis upload;
	string name = Utils::format_string("%s-%d.%s", machine_name.c_str(), camnum, capturefile_ext.c_str());

	Logger::log(camnum, "--------------------------------------------------------");
	Logger::log(camnum, "Start Upload FTP : %s", name.c_str());
	Logger::log(camnum, "--------------------------------------------------------");

	char* inbuf = NULL;
	int len = 0;

	FILE* fp = NULL;
	fp = fopen(name.c_str(), "rb");
	if (fp == NULL)
	{
		Logger::log(camnum, "%s file not found.", name.c_str());
		camera_state[camnum] = CAMERA_STATE::STATE_READY;
		return false;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	inbuf = new char[len];
	fread(inbuf, 1, len, fp);
	fclose(fp);

	upload[camnum].camnum = camnum;
	upload[camnum].readptr = inbuf;
	upload[camnum].totalsize = len;
	upload[camnum].sizeleft = len;

	Logger::log(camnum, "filesize : %d",len);


	curl = curl_easy_init();
	if (curl)
	{
		string url = "ftp://" + server_address + "/"+ ftp_path + "/" + name;
		string ftpstr = ftp_id +":" + ftp_passwd;

		Logger::log(camnum, "FTP full path : %s (%s)", url.c_str(), ftpstr.c_str());

		//string url = "ftp://192.168.29.103/" + name;
		//curl_easy_setopt(curl, CURLOPT_URL, "ftp://192.168.29.103/2.jpg");
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_USERPWD, ftpstr.c_str());// "codesafe:6502");
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload[camnum]);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)upload[camnum].sizeleft);

		// 전송!
		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			Logger::log(camnum, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		curl_easy_cleanup(curl);
		Logger::log(camnum, "Upload complete\n");
	}

	// send finish packet
	char buf[TCP_BUFFER] = { 0, };
	buf[0] = PACKET_UPLOAD_DONE;
	buf[1] = camnum;
	addSendPacket(buf);

	// 끝
	delete[] inbuf;
	return true;
}

size_t camerathread::read_callback(void* ptr, size_t size, size_t nmemb, void* userp)
{
	struct WriteThis* upload = (struct WriteThis*)userp;
	size_t max = size * nmemb;

	if (max < 1)
		return 0;

	if (upload->sizeleft)
	{
		size_t copylen = max;
		if (copylen > upload->sizeleft)
			copylen = upload->sizeleft;
		memcpy(ptr, upload->readptr, copylen);
		upload->readptr += copylen;
		upload->sizeleft -= copylen;

		int progress = (int)(((float)(upload->totalsize - upload->sizeleft) / upload->totalsize) * 100.f);
		int p = (progress / 10);

		if (upload_progress[upload->camnum] != p)
		{
			upload_progress[upload->camnum] = p;
			Logger::log(upload->camnum, "upload_progress %d\n", upload_progress[upload->camnum]);

			//char data[32];
			//memcpy(data, &upload_progress[upload->camnum], sizeof(int));
			//network[upload->camnum].write(PACKET_UPLOAD_PROGRESS, data, 32);
			//network[upload->camnum].update();

			char buf[TCP_BUFFER] = { 0, };
			if (p == 10)
			{
				//buf[0] = PACKET_UPLOAD_DONE;
				//buf[1] = (char)upload->camnum;
				buf[0] = PACKET_UPLOAD_PROGRESS;
				buf[1] = (char)upload->camnum;
				buf[2] = (char)upload_progress[upload->camnum];
			}
			else
			{
				buf[0] = PACKET_UPLOAD_PROGRESS;
				buf[1] = (char)upload->camnum;
				buf[2] = (char)upload_progress[upload->camnum];
			}

			// progress send to server
			addSendPacket(buf);
		}
		//printf("Progress : %d\n", progress);
		return copylen;
	}

	return 0;                          /* no more data left to deliver */
}

void camerathread::addSendPacket(char* buf)
{
	Logger::log(cameraNum, "addSendPacket");

	char* buffer = new char[TCP_BUFFER];
	memcpy(buffer, buf, TCP_BUFFER);

	pthread_mutex_lock(&mutex_lock[cameraNum]);
	packetbuffer[cameraNum].push_back(buffer);
	pthread_mutex_unlock(&mutex_lock[cameraNum]);
}

bool camerathread::getSendPacket(int camnumber, char *buf)
{
	bool ret = true;
	pthread_mutex_lock(&mutex_lock[camnumber]);

	if (packetbuffer[camnumber].empty() == false)
	{
		char* b = packetbuffer[camnumber][0];
		memcpy(buf, b, TCP_BUFFER);
		delete[] b;
		// remove
		packetbuffer[camnumber].pop_front();
	}
	else
		ret = false;
	pthread_mutex_unlock(&mutex_lock[camnumber]);
	return ret;
}

void camerathread::wakeup(int camnumber)
{
	Logger::log("----> Wake up : %d", camnumber);
	pthread_cond_signal(&cond[camnumber]);
}
