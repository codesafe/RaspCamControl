#include "camera_manager.h"
#include "camcontrol.h"

#if 1

camera_manager* camera_manager::_instance = nullptr;

camera_manager::camera_manager()
{

}

camera_manager::~camera_manager()
{

}

void camera_manager::enumCameraList()
{
	auto cameraList = gphoto2pp::autoDetectAll();
	for (int i = 0; i < cameraList.count(); ++i)
	{
		camerainfo _camerainfo;
		_camerainfo.modelname = cameraList.getName(i);
		_camerainfo.port = cameraList.getValue(i);
		Logger::log("Detect Camera %d : %s : %s",i , _camerainfo.modelname.c_str(), _camerainfo.port.c_str());
		cameraIDlist.push_back(_camerainfo);
	}
}

int camera_manager::getEnumCameraNum()
{
	return (int)cameraIDlist.size();
}

camerainfo* camera_manager::getCameraInfo(int i)
{
	return &cameraIDlist[i];
}



#else

camera_manager* camera_manager::_instance = NULL;

camera_manager::camera_manager()
{
	_getCameralist = false;
}

camera_manager::~camera_manager()
{
}

void camera_manager::enumCameraList()
{
	if (_getCameralist == false)
	{
		cameraIDlist.clear();

		//////////////////////////////////////////////////////////////////////////

		Camera* camera = NULL;
		GPContext* context = NULL;
		GPPortInfoList* portinfo_list = NULL;
		CameraAbilitiesList* _abilities_list = NULL;


		gp_camera_new(&camera);
		context = gp_context_new();

		//gp_context_set_error_func(context, (GPContextErrorFunc)Camera_Context::_error_callback, NULL);
		//gp_context_set_status_func(context, (GPContextMessageFunc)Camera_Context::_message_callback, NULL);

		int ret = gp_camera_init(camera, context);
		if (ret < GP_OK)
		{
			Logger::log("gp_camera_init Error!");
			gp_camera_free(camera);
			return;
		}

		//////////////////////////////////////////////////////////////////////////

		int count, result;
		if (gp_port_info_list_new(&portinfo_list) < GP_OK)
			return;

		result = gp_port_info_list_load(portinfo_list);
		if (result < 0)
		{
			gp_port_info_list_free(portinfo_list);
			return;
		}

		count = gp_port_info_list_count(portinfo_list);
		if (count < 0)
		{
			gp_port_info_list_free(portinfo_list);
			return;
		}

		GPPortInfo info;
		for (int x = 0; x < count; x++)
		{
			char* xname, * xpath;
			result = gp_port_info_list_get_info(portinfo_list, x, &info);
			if (result < 0)
				break;
			gp_port_info_get_name(info, &xname);
			gp_port_info_get_path(info, &xpath);
			Logger::log("%-32s %-32s\n", xpath, xname);
		}

		//////////////////////////////////////////////////////////////////////////

		const char* name = NULL, * value = NULL;
		CameraList* list;
		gp_list_new(&list);
		gp_abilities_list_detect(gp_params_abilities_list(context, _abilities_list), portinfo_list, list, context);

		count = gp_list_count(list);

		Logger::log("%-30s %-16s\n", "Model", "Port");
		Logger::log("----------------------------------------------------------\n");

		for (int x = 0; x < count; x++)
		{
			gp_list_get_name(list, x, &name);
			gp_list_get_value(list, x, &value);
			Logger::log("%-30s %-16s\n", name, value);

			camerainfo info;
			info.port = std::string(value);
			info.modelname = std::string(name);
			cameraIDlist.push_back(info);
		}
		gp_list_free(list);
		Logger::log("----------------------------------------------------------\n");

		if(camera != NULL)
			gp_camera_exit(camera, context);

 		if (camera != NULL)
 			gp_camera_free(camera);

		if( context != NULL )
			gp_context_unref(context);

		if (portinfo_list != NULL)
			gp_port_info_list_free(portinfo_list);
		if (portinfo_list != NULL)
			gp_abilities_list_free(_abilities_list);

		_getCameralist = true;
	}

	// create all cameras
	if (_getCameralist)
	{
		for (int i=0; i< cameraIDlist.size(); i++)
		{
			camcontrol* camera = new camcontrol();
			camera->create(cameraIDlist[i]);
			cameraList.push_back(camera);
		}
	}


}

CameraAbilitiesList* camera_manager::gp_params_abilities_list(GPContext* context, CameraAbilitiesList* _abilities_list)
{
	/* If p == NULL, the behaviour of this function is as undefined as
	 * the expression p->abilities_list would have been. */
	if (_abilities_list == NULL)
	{
		gp_abilities_list_new(&_abilities_list);
		gp_abilities_list_load(_abilities_list, context);
	}
	return _abilities_list;
}

// °¹¼ö
int camera_manager::getEnumCameraNum()
{
	return cameraIDlist.size();
}

camcontrol* camera_manager::getCamera(int i)
{
	if (cameraList.size() <= i)
		return NULL;

	return cameraList[i];
}

#endif