#pragma once

#include"CVRender/cvrender.h"
#include<opencv2/highgui.hpp>
#include"opencv2/xfeatures2d.hpp"
#include"opencv2/calib3d.hpp"
using namespace cv;
#include<time.h>
#include<atomic>
using namespace std;

#include"CVX/core.h"
#include"BFC/stdf.h"

#include"base/utils.h"

#include"Base/cmds.h"

_CMDI_BEG

void loadModelData(const std::string &file, std::vector<Point3f> &vpt, Mat &desc)
{
	Mat data = loadMatData(file);
	CV_Assert(data.type() == CV_32FC1);

	vpt.resize(data.rows);
	desc.create(data.rows, data.cols - 3, data.type());

	for (int i = 0; i < data.rows; ++i)
	{
		float *dp = data.ptr<float>(i);
		memcpy(&vpt[i], dp, sizeof(float) * 3);
		memcpy(desc.ptr(i), dp + 3, sizeof(float)*desc.cols);
	}
}

void detect(const Mat3b &img, std::vector<KeyPoint> &kp, Mat &desc, Mat1b mask = Mat1b())
{
	//time_t beg = clock();

	typedef cv::AKAZE FD;
	//typedef cv::xfeatures2d::SIFT FD;

	cv::Ptr<FD> fd(FD::create());
	fd->detect(img, kp, mask);

	//printf("fdet=%d\n", int(clock() - beg));
	//beg = clock();

	auto fd2 = cv::xfeatures2d::SIFT::create();
	fd2->compute(img, kp, desc);

	//printf("fdesc=%d\n", int(clock() - beg));
}

void findMatch(const Mat &camImgDesc, const Mat &objImgDesc, std::vector<DMatch> &matches)
{
	Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("FlannBased");

	matcher->match(camImgDesc, objImgDesc, matches);
}

void getPointPairs(const Mat3b &camImg, const Mat &objImgDesc, const std::vector<Point3f> &objKP,
	std::vector<Point2f> &imgPoints, std::vector<Point3f> &objPoints, int nsel=100)
{
	//time_t beg = clock();

	Mat camImgDesc;
	std::vector<KeyPoint> camImgKP;
	detect(camImg, camImgKP, camImgDesc);

	//printf("t-detect=%d\n", int(clock() - beg)); beg = clock();

	std::vector<DMatch> matches;
	//matcher->match(objImgDesc, camImgDesc, matches);
	findMatch(camImgDesc, objImgDesc, matches);

	std::sort(matches.begin(), matches.end(), [](const DMatch &a, const DMatch &b) {
		return a.distance < b.distance;
	});

	nsel = __min(nsel, (int)matches.size() / 2);
	//matches.resize(matches.size() / 2);
	matches.resize(nsel);

	imgPoints.resize(matches.size());
	objPoints.resize(matches.size());

	for (size_t i = 0; i < matches.size(); ++i)
	{
		auto &m(matches[i]);
		imgPoints[i] = camImgKP[m.queryIdx].pt;
		objPoints[i] = objKP[m.trainIdx];
	}

	//filterOutlierLPM(imgPoints, objPoints);
	//printf("t-match=%d\n", int(clock() - beg));
}

CVRMats solveObjPose(const std::vector<Point2f> &imgPts, const std::vector<Point3f> &objPts, Size viewSize)
{
	cv::Mat1d K = cv::Mat1d::eye(3, 3);
	K.at<double>(0, 0) = 891.5850888305509;
	K.at<double>(1, 1) = 899.7582345985664;
	K.at<double>(0, 2) = 451.2811989801057;
	K.at<double>(1, 2) = 268.8070676988752;
	//K(0, 2) = viewSize.width / 2;
	//K(1, 2) = viewSize.height / 2; 

	cv::Mat distCoeffs;
	/*distCoeffs= cv::Mat(1, 5, CV_64F);
	distCoeffs.at<double>(0, 0) = 0.178265644047564;
	distCoeffs.at<double>(0, 1) = -0.1942515244450349;
	distCoeffs.at<double>(0, 2) = -0.001475393656445968;
	distCoeffs.at<double>(0, 3) = -0.0138682465167786;
	distCoeffs.at<double>(0, 4) = -2.310032357752005;*/

	CVRMats mats;

	Mat rvec, tvec;
	if (!cv::solvePnPRansac(objPts, imgPts, K, distCoeffs, rvec, tvec, false, 1000, 8, 0.99, noArray(), SOLVEPNP_AP3P))
		//	if(!cv::solvePnP(objPts, imgPts, K, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_EPNP))
		return mats;

	mats.mModel = cvrm::fromRT(rvec, tvec);
	mats.mProjection = cvrm::fromK(K, viewSize, 0.1, 2000);

	return mats;
}

class Tracker
{
	CVRModel _model;
	std::vector<Point3f> _objKP;
	Mat _objDesc;
	CVRender _render;
public:
	void load(const std::string &modelFile, const std::string &modelDataFile)
	{
		_model = CVRModel(modelFile);
		loadModelData(modelDataFile, _objKP, _objDesc);
		_render = CVRender(_model);
	}
	void track(const Mat3b &img)
	{
		std::vector<Point2f> imgPt;
		std::vector<Point3f> objPt;
		getPointPairs(img, _objDesc, _objKP, imgPt, objPt,100);

		//time_t beg = clock();
		CVRMats mats = solveObjPose(imgPt, objPt, img.size());
		//printf("t-solve=%d\n", int(clock() - beg));

		_render.setBgImage(img);

		CVRResult result = _render.exec(mats, img.size(), -1, CVRM_ENABLE_ALL);


		imshow("result", result.img);

		//showModelMatches("matches", _model, objPt, img, imgPt);
		//cvxWaitKey();

	}
};


_CMDI_END


