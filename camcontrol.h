#ifndef CAMCONTROL_H
#define CAMCONTROL_H

#if 1

#include <iostream>
#include <vector>
#include <exception>
#include "predef.h"

struct camerainfo
{
	string	modelname;
	string	port;
};

class camcontrol
{
public:
	camcontrol();
	~camcontrol();

	void create(camerainfo& _info);
	void release();

	camerainfo* getInfo();
	void setInfo(camerainfo& info);

	bool is_halfpressed() { return _halfpressed; }
	bool is_busy();
	//void is_bussy(bool busy);
	//int capture(const char* filename);
	//int capture2(const char* filename);
	int capture3(const char* filename);
	int downloadimage(const char* filename);

	int get_settings_value(const char* key, string& val);
	//int get_settings_choices(const char* key, std::vector<string>& choices);
	int set_settings_value(string key, string val);
	//int set_settings_value(const char* key, const char* val);
	//int set_settings_value(const char* key, int val);

	int apply_cancelautofocus(int camnum, bool enable);
	int apply_essential_param_param(int camnum);
	void set_essential_param(CAMERA_PARAM param, string value);

private:

	int doRadioWidget(std::string key, std::string value);
	int doToggleWidget(std::string key, bool value);

	int _find_widget_by_name(GPParams* p, const char* name, CameraWidget** child, CameraWidget** rootconfig);
	int set_config_action(GPParams* p, const char* name, const char* value);

	void mySuperSpecialHandler(const gphoto2pp::CameraFilePathWrapper& cameraFilePath, const std::string& data);
	void completeHandler(const gphoto2pp::CameraFilePathWrapper& cameraFilePath, const std::string& data);

	bool _halfpressed;
	bool _is_busy;
	bool _liveview_running;
	bool _camera_found;
	bool _is_initialized;
	bool _save_images;

	string	savefilename;
	camerainfo	info;
	CameraWrapper* cameraWrapper;
	Camera* _camera;
	GPContext* _context;
};

#else

#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <gphoto2/gphoto2-camera.h>

#include "predef.h"

struct camerainfo
{
	string	modelname;
	string	port;
};

class camcontrol
{
	static GPContextErrorFunc _error_callback(GPContext* context, const char* text, void* data);
	//static GPContextMessageFunc _message_callback(GPContext* context, const char* text, void* data);
	static GPContextStatusFunc _message_callback(GPContext* context, const char* format, va_list args, void* data);

public:
	camcontrol();
	~camcontrol();

	void init();
	void release();

	//string getport();
	camerainfo *getInfo();
	void setInfo(camerainfo &info);
	bool camera_found();
	bool is_initialized();
	bool is_halfpressed() { return _halfpressed; }
	bool is_busy();
	void is_bussy(bool busy);
	int capture(const char* filename);
	int capture2(const char* filename);
	int capture3(const char* filename);
	int downloadimage(const char* filename);

	int get_settings_value(const char* key, string& val);
	int get_settings_choices(const char* key, std::vector<string>& choices);
	int set_settings_value(const char* key, const char* val);
	int set_settings_value(const char* key, int val);

	int apply_autofocus(int camnum, bool enable);
	int apply_essential_param_param(int camnum);
	void set_essential_param(CAMERA_PARAM param, string value);

	//         int preview(const char **file_data);
	//         int bulb(const char *filename, string &data);
	//         int get_settings(ptree &sett);
	//         int get_files(ptree &tree);
	//         int get_file(const char *filename, const char *filepath, string &base64out);

private:

	Camera* _camera;
	GPContext* _context;

	bool _halfpressed;
	bool _is_busy;
	bool _liveview_running;
	bool _camera_found;
	bool _is_initialized;
	bool _save_images;

	// 		string iso;
	// 		string shutterspeed;
	// 		string aperture;
	//string port;
	camerainfo	info;

	int _wait_and_handle_event(useconds_t waittime, CameraEventType* type, int download);
	void _set_capturetarget(int index);

	int _find_widget_by_name(GPParams* p, const char* name, CameraWidget** child, CameraWidget** rootconfig);
	int set_config_action(GPParams* p, const char* name, const char* value);
};


#endif

#endif