#pragma once

#include "predef.h"

class camcontrol;

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

	std::vector<string>				cameraIDlist;
	std::vector<camcontrol*>	cameraList;

};