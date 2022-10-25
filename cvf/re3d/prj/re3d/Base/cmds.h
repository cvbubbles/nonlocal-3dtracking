#pragma once

#include"object.h"
#include"CVX/core.h"
#include"CVX/gui.h"
#include"opencv2/highgui.hpp"
using namespace cv;

#include"CVRender/cvrender.h"


class ICommand
	:public Object
{
public:
	virtual void exec(std::string dataDir="", std::string args = "") = 0;
};


#define _CMDI_BEG  namespace{

#define _CMDI_END }

