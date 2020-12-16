#ifndef CAMCONTROL_H
#define CAMCONTROL_H

#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <gphoto2/gphoto2-camera.h>

#include "predef.h"


class camcontrol
{
	static GPContextErrorFunc _error_callback(GPContext* context, const char* text, void* data);
	static GPContextMessageFunc _message_callback(GPContext* context, const char* text, void* data);

public:
	camcontrol();
	~camcontrol();

	void init();
	void release();

	string getport();
	void setPort(string port);
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
	string port;

	void _init_camera();
	int _wait_and_handle_event(useconds_t waittime, CameraEventType* type, int download);
	void _set_capturetarget(int index);

	int _find_widget_by_name(GPParams* p, const char* name, CameraWidget** child, CameraWidget** rootconfig);
	int set_config_action(GPParams* p, const char* name, const char* value);


};


#endif