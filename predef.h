#ifndef _PREDEF_
#define _PREDEF_

#ifdef __linux__
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <utime.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <memory>
#include <string>
#include <ctime>
#include <deque>
#include <vector>

#include "logger.h"
//#include "tcpsocket.h"
//#include "udpsocket.h"

using namespace std;

#define thread_local __thread

//////////////////////////////////////////////////////////////////////////

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-context.h>
#include <gphoto2/gphoto2-port-log.h>
#include <gphoto2/gphoto2-setting.h>
#include <gphoto2/gphoto2-filesys.h>
#include <gphoto2/gphoto2-port-result.h>


typedef struct _GPParams GPParams;
struct _GPParams 
{
	std::string name;
	Camera* camera;
	GPContext* context;
	GPPortInfoList* portinfo_list;
	CameraAbilitiesList* _abilities_list;

	_GPParams()
	{
		camera = NULL;
		context = NULL;
		portinfo_list = NULL;
		_abilities_list = NULL;
	}
};

enum wait_type 
{
	WAIT_TIME,
	WAIT_EVENTS,
	WAIT_FRAMES,
	WAIT_STRING,
};

enum download_type { DT_NO_DOWNLOAD, DT_DOWNLOAD };
struct waitparams
{
	union 
	{
		int milliseconds;
		int events;
		int frames;
		char* str;
	} u;
	enum wait_type type;
	enum download_type downloadtype;
};

//////////////////////////////////////////////////////////////////////////

#include <curl/curl.h>

struct WriteThis
{
	int camnum;
	char* readptr;
	size_t totalsize;
	size_t sizeleft;

	WriteThis()
	{
		camnum = -1;
		readptr = NULL;
		totalsize = 0;
		sizeleft = 0;
	}
};


//////////////////////////////////////////////////////////////////////////

#define CAPTURE_TO_RAM			0 // "Internal RAM"
#define CAPTURE_TO_SDCARD		1 // "Memory card"


// 카메라 최대 10개
#define MAX_CAMERA	10

// 카메라 상태
enum CAMERA_STATE
{
	STATE_NONE,
	STATE_STARTCONNECT,
	STATE_CONNECTION,
	STATE_CONNECT_ERROR,
	STATE_READY,
	STATE_FOCUSING,
	STATE_SHOT,
	STATE_UPLOAD,
	STATE_UPLOADING,
};



//////////////////////////////////////////////////////////////////////////

//#define	USE_NONEBLOCK

#define PATCHDOWNLOADDIR	"./tempdownload"
#define PATCHSERVER_ADD		"http://127.0.0.1:8000/patch/patch.xml"
#define PATCHFILENAME		"patch.xml"

#define SERVER_ADD			"192.168.29.103"
#define SERVER_TCP_PORT		8888
#define SERVER_UDP_PORT		11000
#define SOCKET_BUFFER		4096
#define TCP_BUFFER			16
#define UDP_BUFFER			32

//////////////////////////////////////////////////////////////////////////	network packet

// response packet
#define RESPONSE_OK		0x05
#define RESPONSE_FAIL	0x06

// Log packet
#define CLIENT_LOG_INFO		0x0a
#define CLIENT_LOG_WARN		0x0b
#define CLIENT_LOG_ERR		0x0c


// Packet
#define	PACKET_TRY_CONNECT		0x05	// connect to server
#define	PACKET_SHOT				0x10	// shot picture
#define PACKET_HALFPRESS		0x20	// auto focus
#define PACKET_HALFRELEASE		0x21	// auto focus cancel

#define PACKET_ISO				0x31
#define PACKET_APERTURE			0x32
#define PACKET_SHUTTERSPEED		0x33
#define PACKET_CAPTURE_FORMAT	0x34

#define PACKET_FORCE_UPLOAD		0x40	// for test
#define PACKET_UPLOAD_PROGRESS	0x41
#define PACKET_UPLOAD_DONE		0x42

#define PACKET_AUTOFOCUS_RESULT	0x50


#define DEFAULT_CAPTURENAME		"capt0000.jpg"

//////////////////////////////////////////////////////////////////////////

enum CAMERA_PARAM
{
	ISO,
	SHUTTERSPEED,
	APERTURE,
	CAPTURE_FORMAT
};

#define ISO_VALUE				3	// 400
#define SHUTTERSPEED_VALUE		36	// 1 / 100
//#define SHUTTERSPEED_VALUE	46	// 1/1000
#define APERTURE_VALUE			5	// 9
#define CAPTURE_FORMAT_VALUE	0

/*

#define ISO_VALUE				"400"
#define SHUTTERSPEED_VALUE		"1/100"
//#define SHUTTERSPEED_VALUE	46	// 1/1000
#define APERTURE_VALUE			"9"
*/

extern string iso;
extern string aperture;
extern string shutterspeed;
extern string captureformat;

extern string apertureString[];
extern string isoString[];
extern string shutterspeedString[];
extern string captureformatString[];

// read from local config.txt
extern string server_address;
extern string machine_name;
extern string capturefile_ext;			// 캡쳐파일 확장자


// recv from server
extern string ftp_path;
extern string camera_id;

extern class TCP_Socket tcp_socket;
//extern class UDP_Socket udp_socket;



#endif
