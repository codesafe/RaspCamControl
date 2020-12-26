
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
#include "udpsocket.h"

///////////////////////////////////////////////////////////////////////////////////////////////

camcontrol* camerathread::cameras[MAX_CAMERA];

bool camerathread::exitthread[MAX_CAMERA];
pthread_mutex_t camerathread::mutex_lock[MAX_CAMERA];
pthread_mutex_t camerathread::exitmutex_lock[MAX_CAMERA];

//pthread_cond_t camerathread::cond[MAX_CAMERA];
//pthread_mutex_t camerathread::mutex[MAX_CAMERA];

int camerathread::upload_progress[MAX_CAMERA];
std::deque<char*> camerathread::packetbuffer[MAX_CAMERA];
char camerathread::recvBuffer[MAX_CAMERA][UDP_BUFFER];
WriteThis camerathread::upload[MAX_CAMERA];

UDP_Socket camerathread::udpsocket[MAX_CAMERA];


//thread_local unsigned int cameraNum;

camerathread::camerathread()
{
}

camerathread::~camerathread()
{
}

void camerathread::init(int camnumber, char macninenum)
{
	pthread_mutex_init(&mutex_lock[camnumber], NULL);
	pthread_mutex_init(&exitmutex_lock[camnumber], NULL);

	info.cameranum = camnumber;
	// 포트 계산 : 11000 + (머신번호*100) + 카메라 번호
	info.udp_port = SERVER_UDP_PORT + (macninenum * 100) + camnumber;

	//int err = 
	pthread_create(&threadid, NULL, thread_fn, (void*)&info);
}


void* camerathread::thread_fn(void* arg)
{
	ThreadPassInfo *info = (ThreadPassInfo*)arg;

	int cameraNum = info->cameranum;
	int udp_port = info->udp_port;

	udpsocket[cameraNum].init(udp_port);

	camerainfo* camerainfo = camera_manager::getInstance()->getCameraInfo(cameraNum);
	cameras[cameraNum] = new camcontrol();
	if (cameras[cameraNum] == nullptr)
	{
		Logger::log(cameraNum, "Thread %d : Camera Create fail", cameraNum);
		return((void*)0);
	}
	else
	{
		Logger::log(cameraNum, "Thread %d : Camera Create success", cameraNum);
	}

	cameras[cameraNum]->create(*camerainfo);

	// main thread loop : Wakeup 시그널이 올때까지 영원히 대기
	while (true)
	{
		Logger::log(cameraNum, "Enter Wait State Camera %s", cameras[cameraNum]->getInfo()->modelname.c_str());

		//pthread_cond_wait(&cond[cameraNum], &mutex[cameraNum]);
		char buf[UDP_BUFFER] = { 0, };
		int ret = udpsocket[cameraNum].update(buf);
		if (ret > 0)
			parsePacket(cameraNum, buf);
		else
			Logger::log(cameraNum, "Recv UDP error");
	}

	pthread_exit((void*)0);
	return((void*)0);
}

int camerathread::parsePacket(int camnum, char* buf)
{
	// char* buf = recvBuffer[camnum];
	char packet = buf[0];
	int ret = 0;
	std::string  date = Utils::getCurrentDateTime();

	switch (packet)
	{
		case PACKET_TRY_CONNECT:
			break;

		case PACKET_SET_PARAMETER:
		{
			char data[TCP_BUFFER] = { 0, };
			data[0] = PACKET_SETPARAMETER_RESULT;

			unsigned char i = buf[1];	// iso
			unsigned char s = buf[2];	// shutterspeed
			unsigned char a = buf[3];	// aperture
			unsigned char f = buf[4];	// format

			Logger::log(camnum, "PACKET_SET_PARAMETER %d : %d : %d : %d", i, s, a, f);

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
				addSendPacket(camnum, data);
				Logger::log(camnum, "PACKET_SET_PARAMETER --> RESPONSE_FAIL");
				return ret;
			}

			data[1] = (char)camnum;
			data[2] = RESPONSE_OK;
			addSendPacket(camnum, data);
			Logger::log(camnum, "PACKET_SET_PARAMETER --> RESPONSE_OK");
		}
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

			// false로 하면 auto focus 안먹음


/*
			ret = cameras[camnum]->apply_cancelautofocus(camnum, false);
			if (ret < GP_OK)
			{
				string errorstr = GetError(ret);
				Logger::log(camnum, "ERR apply_autofocus (False) : %s(%d): %d\n", errorstr.c_str(), ret, camnum);

				data[1] = (char)camnum;
				data[2] = RESPONSE_FAIL;
				addSendPacket(camnum, data);
				return ret;
			}
*/

			Logger::log(camnum, "PACKET_HALFPRESS");
			// 포커스 
			//ret = cameras[camnum]->set_settings_value("Canon EOS Remote Release", "Press 1");
			ret = cameras[camnum]->set_settings_value("eosremoterelease", "Press 1");
			if (ret < GP_OK)
			{
				string errorstr = GetError(ret);
				Logger::log(camnum, "ERR eosremoterelease Press Half : %s(%d): %d\n", errorstr.c_str(), ret, camnum);

				data[1] = (char)camnum;
				data[2] = RESPONSE_FAIL;
				addSendPacket(camnum, data);
				Logger::log(camnum, "PACKET_HALFPRESS --> RESPONSE_FAIL 1");
				return ret;
			}
			else
				Logger::log(camnum, "Press 1 : %d : %d", ret, camnum);


			//ret = cameras[camnum]->set_settings_value("Canon EOS Remote Release", "Release 1");
/*
			ret = cameras[camnum]->set_settings_value("eosremoterelease", "Release 1");
			if (ret < GP_OK)
			{
				string errorstr = GetError(ret);
				Logger::log(camnum, "ERR eosremoterelease Release Full : %s(%d): %d\n", errorstr.c_str(), ret, camnum);

				data[1] = (char)camnum;
				data[2] = RESPONSE_FAIL;
				addSendPacket(camnum, data);
				Logger::log(camnum, "PACKET_HALFPRESS --> RESPONSE_FAIL 2");
				return ret;
			}
			else
				Logger::log(camnum, "Release 1 : %d : %d", ret, camnum);*/

/*
			ret = cameras[camnum]->apply_cancelautofocus(camnum, false);
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
			addSendPacket(camnum, data);
			Logger::log(camnum, "PACKET_HALFPRESS --> RESPONSE_OK");
		}
		break;

		case PACKET_SHOT:
		{
			char data[TCP_BUFFER] = { 0, };
			data[0] = PACKET_SHOT_RESULT;

			Logger::log("---> Shot : %d : %s", camnum, date.c_str());

			// ftp path 읽어야함
			ftp_path = (char*)buf+1;

			// 찍어
			string name = Utils::format_string("%s-%d.%s", machine_name.c_str(), camnum, capturefile_ext.c_str());
			int ret = cameras[camnum]->capture3(name.c_str());
			if (ret < GP_OK)
			{
				Logger::log(camnum, "Shot Error : (%d)", ret);

				data[1] = (char)camnum;
				data[2] = RESPONSE_FAIL;
				return ret;
			}
			else
			{
				std::string  date = Utils::getCurrentDateTime();
				Utils::Sleep(1);
				//int ret = cameras[camnum]->downloadimage(name.c_str());
				data[1] = (char)camnum;
				data[2] = RESPONSE_OK;


// 				ret = cameras[camnum]->apply_autofocus(camnum, false);
// 				if (ret < GP_OK)
// 				{
// 					string errorstr = GetError(ret);
// 					Logger::log(camnum, "ERR apply_autofocus (False) : %s(%d): %d\n", errorstr.c_str(), ret, camnum);
// 				}

				ret = cameras[camnum]->set_settings_value("eosremoterelease", "Release 1");
				if (ret < GP_OK)
				{
					string errorstr = GetError(ret);
					Logger::log(camnum, "ERR eosremoterelease Release Full : %s(%d): %d\n", errorstr.c_str(), ret, camnum);
				}

				StartUpload(camnum);
			}
		}
		break;

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
	}

	// send finish packet
	char buf[TCP_BUFFER] = { 0, };
	buf[0] = PACKET_UPLOAD_DONE;
	buf[1] = camnum;
	addSendPacket(camnum, buf);
	Logger::log(camnum, "Upload complete");

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
			addSendPacket(upload->camnum, buf);
		}
		//printf("Progress : %d\n", progress);
		return copylen;
	}

	return 0;                          /* no more data left to deliver */
}

void camerathread::addSendPacket(int camnum, char* buf)
{
	//Logger::log(cameraNum, "addSendPacket");

	char* buffer = new char[TCP_BUFFER];
	memcpy(buffer, buf, TCP_BUFFER);

	pthread_mutex_lock(&mutex_lock[camnum]);
	packetbuffer[camnum].push_back(buffer);
	pthread_mutex_unlock(&mutex_lock[camnum]);
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
//	Logger::log("----> Wake up : %d", camnumber);
//	pthread_cond_signal(&cond[camnumber]);
}

string camerathread::GetError(int errorcode)
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
