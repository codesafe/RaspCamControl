#pragma once

#if 1

#include "predef.h"

struct camerainfo;
class camera_manager
{
public:
	static camera_manager* getInstance()
	{
		if (_instance == nullptr)
			_instance = new camera_manager();
		return _instance;
	}

	void enumCameraList();
	int getEnumCameraNum();		// °¹¼ö
	camerainfo* getCameraInfo(int i);

private:
	camera_manager();
	~camera_manager();

private:
	static camera_manager* _instance;
	std::vector<camerainfo>		cameraIDlist;
};


#else

#include "predef.h"

class camcontrol;
struct camerainfo;

class camera_manager
{
public:
	static camera_manager* getInstance()
	{
		if (_instance == NULL)
			_instance = new camera_manager();
		return _instance;
	}

	void enumCameraList();
	int getEnumCameraNum();		// °¹¼ö
	camcontrol* getCamera(int i);

private:
	camera_manager();
	~camera_manager();

	CameraAbilitiesList* gp_params_abilities_list(GPContext* context, CameraAbilitiesList* _abilities_list);


private:
	static camera_manager* _instance;

	bool _getCameralist;

	//std::vector<string>			cameraIDlist;
	std::vector<camerainfo>		cameraIDlist;
	std::vector<camcontrol*>	cameraList;

};

#endif