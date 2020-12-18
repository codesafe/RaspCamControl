#include "camcontrol.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <utime.h>
#include "utils.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////


void camcontrol::release()
{
	gp_camera_exit(_camera, _context);
}

camcontrol::camcontrol()
{
	_camera_found = false;
	_is_busy = false;
	_is_initialized = false;
	_liveview_running = false;
	_camera = NULL;
	_context = NULL;
	_save_images = true;
	_halfpressed = false;

	// 	iso = ISO_VALUE;
	// 	shutterspeed = SHUTTERSPEED_VALUE;
	// 	aperture = APERTURE_VALUE;

	// 	iso = ::iso;
	// 	shutterspeed = ::shutterspeed;
	// 	aperture = ::aperture;
}

void camcontrol::init()
{
	if (!_camera_found)
	{
		_init_camera();

		if (_save_images && _camera_found)
			_set_capturetarget(CAPTURE_TO_RAM);
	}
}

string camcontrol::getport()
{
	return port;
}

void camcontrol::setPort(string port)
{
	GPPortInfoList* portinfo_list;
	char verified_port[64] = { 0, };
	strcpy(verified_port, port.c_str());
	this->port = port;


	GPPortInfo portinfo;
	if (gp_port_info_list_new(&portinfo_list) < GP_OK)
	{
		Logger::log(-1, "Error gp_port_info_list_new");
		return;
	}

	int result = gp_port_info_list_load(portinfo_list);
	if (result < 0)
	{
		Logger::log(-1, "Error gp_port_info_list_load");
		gp_port_info_list_free(portinfo_list);
		return;
	}

	int p = gp_port_info_list_lookup_path(portinfo_list, verified_port);
	if (p < GP_OK)
	{
		Logger::log(-1, "Error gp_port_info_list_lookup_path : %d", p);
		return;
	}

	int r = gp_port_info_list_get_info(portinfo_list, p, &portinfo);
	if (r < GP_OK)
	{
		Logger::log(-1, "Error gp_port_info_list_get_info : %d", r);
		return;
	}

	gp_camera_set_port_info(_camera, portinfo);

	gp_port_info_list_free(portinfo_list);
}

void camcontrol::_init_camera()
{
	gp_camera_new(&_camera);
	_context = gp_context_new();
	gp_context_set_error_func(_context, (GPContextErrorFunc)camcontrol::_error_callback, NULL);
	gp_context_set_message_func(_context, (GPContextMessageFunc)camcontrol::_message_callback, NULL);

	int ret = gp_camera_init(_camera, _context);
	if (ret < GP_OK)
	{
		gp_camera_free(_camera);
	}
	else
	{
		_camera_found = true;
	}
	_is_initialized = true;
}

camcontrol::~camcontrol()
{
	gp_camera_exit(_camera, _context);
	gp_context_unref(_context);
}


bool camcontrol::is_busy()
{
	return _is_busy;
}

void camcontrol::is_bussy(bool busy)
{
	_is_busy = busy;
}

bool camcontrol::camera_found()
{
	return _camera_found;
}

bool camcontrol::is_initialized()
{
	return _is_initialized;
}

int camcontrol::capture(const char* filename)
{
	_is_busy = true;

	int ret, fd;
	CameraFile* file;
	CameraFilePath path;

	strcpy(path.folder, "/");
	strcpy(path.name, filename);

	ret = gp_camera_capture(_camera, GP_CAPTURE_IMAGE, &path, _context);
	if (ret != GP_OK)
		return ret;


	if (_save_images == false)
	{
		ret = gp_file_new(&file);
	}
	else {
		fd = open(filename, O_CREAT | O_RDWR, 0644);
		ret = gp_file_new_from_fd(&file, fd);
	}

	if (ret != GP_OK) {
		_is_busy = false;
		return ret;
	}

	ret = gp_camera_file_get(_camera, path.folder, path.name, GP_FILE_TYPE_NORMAL, file, _context);

	if (ret != GP_OK) {
		_is_busy = false;
		return ret;
	}

	if (_save_images == false)
	{
		ret = gp_camera_file_delete(_camera, path.folder, path.name, _context);

		if (ret != GP_OK) {
			_is_busy = false;
			return ret;
		}
	}


	int waittime = 10;
	CameraEventType type;
	void* eventdata;

	while (1)
	{
		gp_camera_wait_for_event(_camera, waittime, &type, &eventdata, _context);

		if (type == GP_EVENT_TIMEOUT) {
			break;
		}
		else if (type == GP_EVENT_CAPTURE_COMPLETE || type == GP_EVENT_FILE_ADDED) {
			waittime = 10;
		}
		else if (type != GP_EVENT_UNKNOWN) {
			printf("Unexpected event received from camera: %d\n", (int)type);
		}
	}

	gp_file_free(file);


	_is_busy = false;
	return true;
}

int camcontrol::capture2(const char* filename)
{
	_is_busy = true;

	int ret, fd;
	CameraFile* file;
	CameraFilePath path;

	strcpy(path.folder, "/");
	strcpy(path.name, filename);

	ret = gp_camera_capture(_camera, GP_CAPTURE_IMAGE, &path, _context);
	if (ret != GP_OK)
		return ret;

	_is_busy = false;
	return true;
}

int camcontrol::capture3(const char* filename)
{
	_is_busy = true;

	set_settings_value("eosremoterelease", "Immediate");

	void* data = NULL;
	CameraEventType	event;
	CameraFilePath* fn;
	int ret;
	bool loop = true;

	while (loop)
	{
		int leftoverms = 1000;
		ret = gp_camera_wait_for_event(_camera, leftoverms, &event, &data, _context);
		if (ret != GP_OK)
			return ret;

		switch (event)
		{
		case GP_EVENT_FILE_ADDED:
		{
			fn = (CameraFilePath*)data;

			CameraFile* file;
			CameraFileInfo info;
			ret = gp_camera_file_get_info(_camera, fn->folder, fn->name, &info, _context);
			if (ret != GP_OK)
			{
				printf("gp_camera_file_get_info %s : %d Error\n", filename, ret);

				free(data);
				_is_busy = false;
				return ret;
			}
			else
			{
				printf("gp_camera_file_get_info %s : %s success\n", fn->folder, fn->name);
			}

			int fd;
			fd = open(filename, O_CREAT | O_RDWR, 0644);
			ret = gp_file_new_from_fd(&file, fd);
			if (ret != GP_OK)
			{
				printf("gp_file_new_from_fd %s Error\n", filename);
				gp_file_unref(file);
				free(data);
				_is_busy = false;
				return ret;
			}

			ret = gp_camera_file_get(_camera, fn->folder, fn->name, GP_FILE_TYPE_NORMAL, file, _context);
			if (ret != GP_OK)
			{
				printf("gp_camera_file_get %s : %d Error\n", filename, ret);
				gp_file_unref(file);
				free(data);
				_is_busy = false;
				return ret;
			}
			else
			{
				printf("gp_camera_file_get %s : %s success\n", fn->folder, fn->name);
			}

			//gp_file_unref(file);
			gp_file_free(file);
			//loop = false;
		}
		break;

		case GP_EVENT_CAPTURE_COMPLETE:
		{
			loop = false;
			printf("Capture %s Done!\n", filename);
		}
		break;
		}
		free(data);
	}

	set_settings_value("eosremoterelease", "Release Full");
	_is_busy = false;
	_halfpressed = false;

	return true;
}

int camcontrol::downloadimage(const char* filename)
{
	_is_busy = true;

	int ret, fd;
	CameraFile* file;
	CameraFilePath path;

	strcpy(path.folder, "/");
	strcpy(path.name, filename);

	/*
		ret = gp_camera_capture(_camera, GP_CAPTURE_IMAGE, &path, _context);
		if (ret != GP_OK)
			return ret;
	*/


	if (_save_images == false)
	{
		ret = gp_file_new(&file);
	}
	else
	{
		fd = open(filename, O_CREAT | O_RDWR, 0644);
		ret = gp_file_new_from_fd(&file, fd);
	}

	if (ret != GP_OK) {
		_is_busy = false;
		return ret;
	}

	ret = gp_camera_file_get(_camera, path.folder, path.name, GP_FILE_TYPE_NORMAL, file, _context);

	if (ret != GP_OK) {
		_is_busy = false;
		return ret;
	}

	if (_save_images == false)
	{
		ret = gp_camera_file_delete(_camera, path.folder, path.name, _context);

		if (ret != GP_OK) {
			_is_busy = false;
			return ret;
		}
	}


	int waittime = 10;
	CameraEventType type;
	void* eventdata;

	while (1)
	{
		gp_camera_wait_for_event(_camera, waittime, &type, &eventdata, _context);

		if (type == GP_EVENT_TIMEOUT) {
			break;
		}
		else if (type == GP_EVENT_CAPTURE_COMPLETE || type == GP_EVENT_FILE_ADDED) {
			waittime = 10;
		}
		else if (type != GP_EVENT_UNKNOWN) {
			printf("Unexpected event received from camera: %d\n", (int)type);
		}
	}

	gp_file_free(file);

	_is_busy = false;
}


int camcontrol::get_settings_choices(const char* key, vector<string>& choices)
{
	_is_busy = true;
	CameraWidget* w, * child;
	int ret;

	ret = gp_camera_get_config(_camera, &w, _context);
	if (ret < GP_OK) {
		_is_busy = false;
		return ret;
	}

	ret = gp_widget_get_child_by_name(w, key, &child);
	if (ret < GP_OK)
	{
		_is_busy = false;
		return ret;
	}

	int count = gp_widget_count_choices(child);
	for (int i = 0; i < count; i++) {
		const char* choice;
		ret = gp_widget_get_choice(child, i, &choice);

		if (ret < GP_OK)
		{
			_is_busy = false;
			return ret;
		}

		choices.push_back(string(choice));
	}
	_is_busy = false;
	return true;
}

int camcontrol::get_settings_value(const char* key, string& val)
{
	_is_busy = true;
	CameraWidget* w, * child;
	int ret;

	ret = gp_camera_get_config(_camera, &w, _context);
	if (ret < GP_OK)
	{
		_is_busy = false;
		return ret;
	}

	ret = gp_widget_get_child_by_name(w, key, &child);
	if (ret < GP_OK)
	{
		_is_busy = false;
		return ret;
	}

	void* item_value;
	ret = gp_widget_get_value(child, &item_value);
	if (ret < GP_OK)
	{
		_is_busy = false;
		return ret;
	}

	unsigned char* value = static_cast<unsigned char*>(item_value);
	val.append((char*)value);

	_is_busy = false;
	return true;
}

int camcontrol::set_settings_value(const char* key, const char* val)
{
	if (key == std::string("eosremoterelease") && val == std::string("Press Half"))
		_halfpressed = true;
	else if (key == std::string("eosremoterelease") && val == std::string("Release Full"))
		_halfpressed = false;


	GPParams gp_params;

	gp_params.camera = _camera;
	gp_params.context = _context;

	_is_busy = true;
	int ret = set_config_action(&gp_params, key, val);
	if (ret < GP_OK)
	{
		_is_busy = false;
		return ret;
	}

	_is_busy = true;

	/*
		_is_busy = true;
		CameraWidget* w, * child;
		int ret = gp_camera_get_config(_camera, &w, _context);
		if (ret < GP_OK)
		{
			_is_busy = false;
			return ret;
		}

		ret = gp_widget_get_child_by_name(w, key, &child);
		if (ret < GP_OK)
		{
			_is_busy = false;
			return ret;
		}

		ret = gp_widget_set_value(child, val);
		if (ret < GP_OK)
		{
			_is_busy = false;
			return ret;
		}


		ret = gp_camera_set_config(_camera, w, _context);
		if (ret < GP_OK)
		{
			_is_busy = false;
			return ret;
		}
		gp_widget_free(w);

		_is_busy = false;*/
	return ret;
}

int camcontrol::set_settings_value(const char* key, int val)
{
	_is_busy = true;
	CameraWidget* w, * child;
	int ret = gp_camera_get_config(_camera, &w, _context);
	if (ret < GP_OK)
	{
		_is_busy = false;
		return ret;
	}

	ret = gp_widget_get_child_by_name(w, key, &child);
	if (ret < GP_OK)
	{
		_is_busy = false;
		return ret;
	}

	ret = gp_widget_set_value(child, &val);
	if (ret != GP_OK)
	{
		_is_busy = false;
		return ret;
	}


	ret = gp_camera_set_config(_camera, w, _context);
	if (ret < GP_OK)
	{
		_is_busy = false;
		return ret;
	}

	gp_widget_free(w);

	_is_busy = false;
	return ret;
}


/**
 borrowed gphoto2
 http://sourceforge.net/p/gphoto/code/HEAD/tree/trunk/gphoto2/gphoto2/main.c#l834
 */
int camcontrol::_wait_and_handle_event(useconds_t waittime, CameraEventType* type, int download)
{
	int 		result;
	CameraEventType	evtype;
	void* data;
	CameraFilePath* path;

	if (!type) type = &evtype;
	evtype = GP_EVENT_UNKNOWN;
	data = NULL;
	result = gp_camera_wait_for_event(_camera, waittime, type, &data, _context);
	if (result == GP_ERROR_NOT_SUPPORTED)
	{
		*type = GP_EVENT_TIMEOUT;
		//usleep(waittime*1000);
		Utils::Sleep(waittime * 1000);
		return GP_OK;
	}

	if (result != GP_OK)
		return result;
	path = (CameraFilePath*)data;
	switch (*type) {
	case GP_EVENT_TIMEOUT:
		break;
	case GP_EVENT_CAPTURE_COMPLETE:
		break;
	case GP_EVENT_FOLDER_ADDED:
		free(data);
		break;
	case GP_EVENT_FILE_ADDED:
		//result = save_captured_file (path, download);
		free(data);
		/* result will fall through to final return */
		break;
	case GP_EVENT_UNKNOWN:
		free(data);
		break;
	default:
		break;
	}
	return result;
}

void camcontrol::_set_capturetarget(int index)
{
	std::vector<string> choices;

	int ret;
	ret = get_settings_choices("capturetarget", choices);

	for (int i = 0; i < choices.size(); i++)
	{
		printf("Store type : %s\n", choices[i].c_str());
	}

	if (ret && index < choices.size())
	{
		string choice = choices.at(index);
		set_settings_value("capturetarget", choice.c_str());
	}

}

GPContextErrorFunc camcontrol::_error_callback(GPContext* context, const char* text, void* data) {

	return 0;
}

GPContextMessageFunc camcontrol::_message_callback(GPContext* context, const char* text, void* data) {

	return 0;
}

int camcontrol::apply_autofocus(int camnum, bool enable)
{
	int ret = set_settings_value("cancelautofocus", enable ? "0" : "1");
	if (ret < GP_OK)
	{
		Logger::log(camnum, "Error cancelautofocus : %d", ret);
		return ret;
	}

	//ret = set_settings_value("autofocusdrive", enable ? "True" : "False");
	ret = set_settings_value("autofocusdrive", enable ? "1" : "0");
	if (ret < GP_OK)
	{
		Logger::log(camnum, "Error autofocusdrive : %d", ret);
		return ret;
	}


	return ret;
}

// 아래의 param 셋팅
int camcontrol::apply_essential_param_param(int camnum)
{
	int ret = 0;
	// 	ret = set_settings_value("autofocusdrive", "0");
	// 	if (ret < GP_OK)
	// 	{
	// 		Logger::log(0, "Error autofocusdrive : %d : ret", camnum, ret);
	// 		return ret;
	// 	}

	/*
		ret = set_settings_value("cancelautofocus", "1");
		if (ret < GP_OK)
		{
			Logger::log(0, "Error cancelautofocus : %d : ret", camnum, ret);
			return ret;
		}*/

	Logger::log(camnum, "apply iso : %s", iso.c_str());
	ret = set_settings_value("iso", iso.c_str());					// "400"
	if (ret < GP_OK)
	{
		Logger::log(camnum, "Error set_settings_value iso : %s : %d : %d", iso.c_str(), camnum, ret);
		return ret;
	}

	Logger::log(camnum, "apply aperture : %s", aperture.c_str());
	ret = set_settings_value("aperture", aperture.c_str());				// "10"
	if (ret < GP_OK)
	{
		Logger::log(camnum, "Error set_settings_value aperture : %s : %d : %d", aperture.c_str(), camnum, ret);
		return ret;
	}

	Logger::log(camnum, "apply shutterspeed : %s", shutterspeed.c_str());
	ret = set_settings_value("shutterspeed", shutterspeed.c_str());		// "1/100"
	if (ret < GP_OK)
	{
		Logger::log(camnum, "Error set_settings_value shutterspeed : %s : %d : %d", shutterspeed.c_str(), camnum, ret);
		return ret;
	}

	Logger::log(camnum, "apply imageformat : %s", captureformat.c_str());
	ret = set_settings_value("imageformat", captureformat.c_str());		// "RAW"
	if (ret < GP_OK)
	{
		Logger::log(camnum, "Error set_settings_value imageformat : %s : %d : %d", captureformat.c_str(), camnum, ret);
		return ret;
	}


	return ret;
}

void camcontrol::set_essential_param(CAMERA_PARAM param, string value)
{
	switch (param)
	{
	case ISO:
		iso = value;
		Logger::log("recv setting iso  %s", iso.c_str());
		break;
	case SHUTTERSPEED:
		shutterspeed = value;
		Logger::log("recv setting shutterspeed  %s", shutterspeed.c_str());
		break;
	case APERTURE:
		aperture = value;
		Logger::log("recv setting aperture  %s", aperture.c_str());
		break;

	case CAPTURE_FORMAT:
		captureformat = value;
		if (captureformat == "RAW + Large Fine JPEG" || captureformat == "RAW")
			capturefile_ext = "raw";
		else
			capturefile_ext = "jpg";
		Logger::log("recv setting captureformat  %s", captureformat.c_str());
		break;
	}
}



int camcontrol::_find_widget_by_name(GPParams* p, const char* name, CameraWidget** child, CameraWidget** rootconfig)
{
	int	ret;

	*rootconfig = NULL;
	ret = gp_camera_get_single_config(p->camera, name, child, p->context);
	if (ret == GP_OK) {
		*rootconfig = *child;
		return GP_OK;
	}

	ret = gp_camera_get_config(p->camera, rootconfig, p->context);
	if (ret != GP_OK) return ret;
	ret = gp_widget_get_child_by_name(*rootconfig, name, child);
	if (ret != GP_OK)
		ret = gp_widget_get_child_by_label(*rootconfig, name, child);
	if (ret != GP_OK)
	{
		char* part, * s, * newname;

		newname = strdup(name);
		if (!newname)
			return GP_ERROR_NO_MEMORY;

		*child = *rootconfig;
		part = newname;
		while (part[0] == '/')
			part++;
		while (1)
		{
			CameraWidget* tmp;

			s = strchr(part, '/');
			if (s)
				*s = '\0';
			ret = gp_widget_get_child_by_name(*child, part, &tmp);
			if (ret != GP_OK)
				ret = gp_widget_get_child_by_label(*child, part, &tmp);
			if (ret != GP_OK)
				break;
			*child = tmp;
			if (!s)
			{
				/* end of path */
				free(newname);
				return GP_OK;
			}
			part = s + 1;
			while (part[0] == '/')
				part++;
		}
		gp_context_error(p->context, "%s not found in configuration tree.", newname);
		free(newname);
		gp_widget_free(*rootconfig);
		return GP_ERROR;
	}
	return GP_OK;
}


int camcontrol::set_config_action(GPParams* p, const char* name, const char* value)
{
	CameraWidget* rootconfig, * child;
	int	ret, ro;
	CameraWidgetType	type;

	ret = _find_widget_by_name(p, name, &child, &rootconfig);
	if (ret != GP_OK)
		return ret;

	ret = gp_widget_get_readonly(child, &ro);
	if (ret != GP_OK)
	{
		gp_widget_free(rootconfig);
		return ret;
	}
	if (ro == 1)
	{
		gp_context_error(p->context, "Property %s is read only.", name);
		gp_widget_free(rootconfig);
		return GP_ERROR;
	}
	ret = gp_widget_get_type(child, &type);
	if (ret != GP_OK) {
		gp_widget_free(rootconfig);
		return ret;
	}

	switch (type)
	{
	case GP_WIDGET_TEXT:
	{		/* char *		*/
		ret = gp_widget_set_value(child, value);
		if (ret != GP_OK)
			gp_context_error(p->context, "Failed to set the value of text widget %s to %s.", name, value);
		break;
	}
	case GP_WIDGET_RANGE:
	{	/* float		*/
		float	f, t, b, s;

		ret = gp_widget_get_range(child, &b, &t, &s);
		if (ret != GP_OK)
			break;
		if (!sscanf(value, "%f", &f)) {
			gp_context_error(p->context, "The passed value %s is not a floating point value.", value);
			ret = GP_ERROR_BAD_PARAMETERS;
			break;
		}
		if ((f < b) || (f > t)) {
			gp_context_error(p->context, "The passed value %f is not within the expected range %f - %f.", f, b, t);
			ret = GP_ERROR_BAD_PARAMETERS;
			break;
		}
		ret = gp_widget_set_value(child, &f);
		if (ret != GP_OK)
			gp_context_error(p->context, "Failed to set the value of range widget %s to %f.", name, f);
		break;
	}
	case GP_WIDGET_TOGGLE:
	{	/* int		*/
		int	t;

		t = 2;
		if (!strcasecmp(value, "off") || !strcasecmp(value, "no") ||
			!strcasecmp(value, "false") || !strcmp(value, "0") ||
			!strcasecmp(value, "off") || !strcasecmp(value, "no") ||
			!strcasecmp(value, "false")
			)
			t = 0;
		if (!strcasecmp(value, "on") || !strcasecmp(value, "yes") ||
			!strcasecmp(value, "true") || !strcmp(value, "1") ||
			!strcasecmp(value, "on") || !strcasecmp(value, "yes") ||
			!strcasecmp(value, "true")
			)
			t = 1;
		/*fprintf (stderr," value %s, t %d\n", value, t);*/
		if (t == 2) {
			gp_context_error(p->context, "The passed value %s is not a valid toggle value.", value);
			ret = GP_ERROR_BAD_PARAMETERS;
			break;
		}
		ret = gp_widget_set_value(child, &t);
		if (ret != GP_OK)
			gp_context_error(p->context, "Failed to set values %s of toggle widget %s.", value, name);
		break;
	}

	case GP_WIDGET_DATE:
	{		/* int			*/
		time_t	t = -1;
		struct tm xtm;

		memset(&xtm, 0, sizeof(xtm));

		/* We need to set UNIX time in seconds since Epoch */
		/* We get ... local time */

		if (!strcasecmp(value, "now") || !strcasecmp(value, "now"))
			t = time(NULL);
#ifdef HAVE_STRPTIME
		else if (strptime(value, "%c", &xtm) || strptime(value, "%Ec", &xtm)) {
			xtm.tm_isdst = -1;
			t = mktime(&xtm);
		}
#endif
		if (t == -1)
		{
			unsigned long lt;

			if (!sscanf(value, "%ld", &lt))
			{
				gp_context_error(p->context, "The passed value %s is neither a valid time nor an integer.", value);
				ret = GP_ERROR_BAD_PARAMETERS;
				break;
			}
			t = lt;
		}
		ret = gp_widget_set_value(child, &t);
		if (ret != GP_OK)
			gp_context_error(p->context, "Failed to set new time of date/time widget %s to %s.", name, value);
		break;
	}
	case GP_WIDGET_MENU:
	case GP_WIDGET_RADIO:
	{ /* char *		*/
		int cnt, i;
		char* endptr;

		cnt = gp_widget_count_choices(child);
		if (cnt < GP_OK) {
			ret = cnt;
			break;
		}
		ret = GP_ERROR_BAD_PARAMETERS;
		for (i = 0; i < cnt; i++) {
			const char* choice;

			ret = gp_widget_get_choice(child, i, &choice);
			if (ret != GP_OK)
				continue;
			if (!strcmp(choice, value)) {
				ret = gp_widget_set_value(child, value);
				break;
			}
		}
		if (i != cnt)
			break;

		/* make sure we parse just 1 integer, and there is nothing more.
		 * sscanf just does not provide this, we need strtol.
		 */
		i = strtol(value, &endptr, 10);
		if ((value != endptr) && (*endptr == '\0')) {
			if ((i >= 0) && (i < cnt)) {
				const char* choice;

				ret = gp_widget_get_choice(child, i, &choice);
				if (ret == GP_OK)
					ret = gp_widget_set_value(child, choice);
				break;
			}
		}
		/* Lets just try setting the value directly, in case we have flexible setters,
		 * like PTP shutterspeed. */
		ret = gp_widget_set_value(child, value);
		if (ret == GP_OK)
			break;
		gp_context_error(p->context, "Choice %s not found within list of choices.", value);
		break;
	}

	/* ignore: */
	case GP_WIDGET_WINDOW:
	case GP_WIDGET_SECTION:
	case GP_WIDGET_BUTTON:
		gp_context_error(p->context, "The %s widget is not configurable.", name);
		ret = GP_ERROR_BAD_PARAMETERS;
		break;
	}
	if (ret == GP_OK)
	{
		if (child == rootconfig)
			ret = gp_camera_set_single_config(p->camera, name, child, p->context);
		else
			ret = gp_camera_set_config(p->camera, rootconfig, p->context);
		if (ret != GP_OK)
			gp_context_error(p->context, "Failed to set new configuration value %s for configuration entry %s.", value, name);
	}
	gp_widget_free(rootconfig);
	return (ret);
}

