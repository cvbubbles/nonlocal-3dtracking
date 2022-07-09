#pragma once

#include"tracker_base.h"
#include"utils.h"
#include"CVX/vis.h"
#include<fstream>

inline std::vector<Pose> read_gt_poses(const std::string& file)
{
	std::ifstream is(file);
	if (!is)
		throw file;

	std::string tstr;
	std::getline(is, tstr); //ignore the first line

	std::vector<Pose> poses;
	Pose p;
	while (is)
	{
		for (int i = 0; i < 9; ++i)
			is >> p.R.val[i];
		for (int i = 0; i < 3; ++i)
			is >> p.t.val[i];
		if (is)
			poses.push_back(p);
	}
	return poses;
}

inline float get_errorR(cv::Matx33f R_gt, cv::Matx33f R_pred) {
	cv::Matx33f tmp = R_pred.t() * R_gt;
	float trace = tmp(0, 0) + tmp(1, 1) + tmp(2, 2);
	return acos((trace - 1) / 2) * 180 / 3.14159265;
}

inline float get_errort(cv::Vec3f t_gt, cv::Vec3f t_pred) {
	float l22 = pow(t_gt[0] - t_pred[0], 2) + pow(t_gt[1] - t_pred[1], 2) + pow(t_gt[2] - t_pred[2], 2);
	return sqrt(l22);
}

inline void printLost(const Matx33f& R, const Vec3f& t, const Matx33f& R0, const Vec3f& t0)
{
	float errR = get_errorR(R, R0);
	float errt = get_errort(t, t0);

	printf("errR=%.2f, errt=%.2f   \n", errR, errt);
}

inline bool isLost(const Matx33f& R, const Vec3f& t, const Matx33f& R0, const Vec3f& t0)
{
	float errR = get_errorR(R, R0);
	float errt = get_errort(t, t0);

	if (errR > 5.f || errt > 50.f)
	//if (errR > 2.f || errt > 20.f)
	{
		return true;
	}

	return false;
}

inline bool isLost(const Pose& pose, const Pose& gt)
{
	return isLost(pose.R, pose.t, gt.R, gt.t);
}

inline Matx33f getRBOT_K()
{
	cv::Matx33f K = cv::Matx33f::zeros();
	K(0, 0) = 650.048;
	K(0, 2) = 324.328;
	K(1, 1) = 647.183;
	K(1, 2) = 257.323;
	K(2, 2) = 1.0f;
	return K;
}




class VisHandler
	:public UpdateHandler
{
public:
	CVRModel _model;
	CVRender _render;
	cv::Matx33f _K;
	Mat     _img;
	std::string _name;
	Mat     resultImg;
public:
	VisHandler(CVRModel& model, const cv::Matx33f& K)
		:_model(model), _render(_model), _K(K)
	{
	}
	void setBgImage(Mat img, std::string name = "vishdl")
	{
		_img = cv::convertBGRChannels(img, 3);
		_name = name;
	}

	void onUpdate(const cv::Matx33f& R, const cv::Vec3f& t, const UpdateHandler::Infos* infos = NULL)
	{
		Mat dimg = _img.clone();
		CVRMats mats;
		mats.mProjection = cvrm::fromK(_K, _img.size(), 1.f, 3000.f);
		mats.mModel = cvrm::fromR33T(R, t);

		auto rr = _render.exec(mats, dimg.size());
		Mat1b mask = getRenderMask(rr.depth);
		/*std::vector<std::vector<Point>> contours;
		cv::findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);*/
		dimg = cv::drawContours(dimg, mask);
		resultImg = dimg;
		imshow(_name, dimg);
	}

	void draw(Mat img, const cv::Matx33f& R, const cv::Vec3f& t)
	{
		setBgImage(img);
		onUpdate(R, t);
	}
};

inline Mat drawPose(std::string wndName, Mat img, CVRModel& model, Matx33f K, Matx33f R, Vec3f t)
{
	VisHandler hdl(model, K);
	hdl.setBgImage(img, wndName);
	hdl.onUpdate(R, t);
	return hdl.resultImg;
}

