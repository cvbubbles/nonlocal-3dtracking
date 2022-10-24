#pragma once

#include"CVX/core.h"
using namespace cv;

class UpdateHandler
{
public:
	struct Infos
	{

	};
public:
	virtual void onUpdate(const cv::Matx33f& R, const cv::Vec3f& t, const Infos* infos = nullptr) = 0;

	virtual ~UpdateHandler() {}
};


struct Pose
{
	cv::Matx33f R;
	cv::Vec3f   t;
};

class ITracker
{
protected:
	Mat _mask;
public:
	virtual void loadModel(const std::string& modelFile, const std::string &args) = 0;

	virtual void setUpdateHandler(UpdateHandler* hdl) = 0;

	virtual void reset(const Mat& img, const Pose& pose, const Matx33f& K) =0;

	virtual void setMask(const Mat &mask)
	{
		_mask = mask;
	}

	virtual void startUpdate(const Mat& img, Pose gtPose=Pose()) = 0;

	virtual float update(Pose& pose) = 0;

	virtual void endUpdate(const Pose& pose) = 0;

	virtual ~ITracker(){}
};

#define _TRACKER_BEG(vx) namespace tracker_##vx{
#define _TRACKER_END() }
