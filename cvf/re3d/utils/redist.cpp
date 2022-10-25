#define CVRENDER_NO_GUI
#include"CVRender/cvrender.h"
#include"opencv2/imgproc.hpp"
#include"opencv2/highgui.hpp"
using namespace cv;
#include"redist.h"
#include<iostream>
using namespace std;
#include"BFC/err.h"
#include"CVX/core.h"

namespace redist {

	static auto  _tbeg = std::chrono::system_clock::now();
	using namespace std;
	using namespace chrono;

	double elapsed()
	{
		auto now = std::chrono::system_clock::now();
		auto duration = duration_cast<microseconds>(now - _tbeg);
		double t = (double(duration.count()) / microseconds::period::den);
		return t;
	}

	void _getRenderMask(const float *depth, int width, int height, int dstride, uchar *mask, int mstep, float eps)
	{
		for(int y=0;y<height;++y, depth+=dstride, mask+=mstep)
			for (int x = 0; x < width; ++x)
			{
				mask[x] = fabs(1.0f - depth[x])<eps ? 0 : 255;
			}
	}
	Mat1b getRenderMask(const Mat1f &depth, float eps=1e-6f)
	{
		Mat1b mask = Mat1b::zeros(depth.size());
		_getRenderMask(depth.ptr<float>(), depth.cols, depth.rows, depth.step1(), mask.data, mask.step, eps);
		return mask;
	}

	void drawFrame(Mat &dimg, const cv::Point2f _oxyz[4])
	{
		cv::Point oxyz[4];
		for (int i = 0; i < 4; ++i)
			oxyz[i] = cv::Point(_oxyz[i]);

		auto drawAxis = [&dimg](cv::Point start, cv::Point end, cv::Scalar color) {
			cv::line(dimg, start, end, color, 2, CV_AA);
		};

		drawAxis(oxyz[0], oxyz[1], Scalar(255, 0, 0));
		drawAxis(oxyz[0], oxyz[2], Scalar(0, 255, 0));
		drawAxis(oxyz[0], oxyz[3], Scalar(0, 0, 255));
	}

	Mat renderResults(const cv::Mat &img, cv::Mat1f& returnDepth, FrameData &fd, ModelSet &ms, bool _drawContour , bool _drawBlend , bool _drawFrame, bool _drawScore)
	{
		cv::Mat dimg = img.clone();

		for (auto &r : fd.objs)
		{
			if (r.score == 0.f /*|| r.roi.empty()*/)
				continue;

			auto pose = r.pose.get<std::vector<RigidPose>>().front();

			CVRMats mats;
			mats.mModel =  cvrm::fromR33T(pose.R, pose.t);
			mats.mProjection = cvrm::fromK(fd.cameraK, img.size(), 0.1, 3000);

			//cout << mats.modelView() << endl;

			auto modelPtr = ms.getModel(r.modelIndex);
			
		//	mats = CVRMats(modelPtr->get3D(), img.size());
		//	cout << mats.modelView() << endl;

			if (_drawContour || _drawBlend)
			{
				CVRModel &m3d = modelPtr->get3DModel();
				CVRender render(m3d);
				//auto rr = render.exec(mats, img.size(), CVRM_IMAGE | CVRM_DEPTH, CVRM_DEFAULT, nullptr, r.roi);
				auto rr = render.exec(mats, img.size(), CVRM_IMAGE | CVRM_DEPTH, CVRM_DEFAULT, nullptr);
				//cv::imshow("rr", rr.img);
				Mat1b mask = getRenderMask(rr.depth);
				returnDepth = rr.depth;
				Rect roi = cv::get_mask_roi(DWHS(mask), 127);

				if (roi.empty())
					continue;
				
				if (_drawBlend)
				{
					Mat t;
					cv::addWeighted(dimg(roi), 0.5, rr.img(roi), 0.5, 0, t);
					t.copyTo(dimg(roi), mask(roi));
				}
				if (_drawContour)
				{
					std::vector<std::vector<Point> > cont;
					cv::findContours(mask, cont, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
					drawContours(dimg, cont, -1, Scalar(255, 0, 0), 2, CV_AA);
				}
			}
			/*if (_drawFrame)
			{
				auto mi = modelPtr->getObject<re3d::MeshInfo>();

				cv::Point3f oxyz[4];
				mi->getFrameR0(oxyz);

				CVRProjector prj(mats, img.size());
				cv::Point2f oxyzPrj[4];
				prj.project(oxyz, oxyzPrj, 4);

				drawFrame(dimg, oxyzPrj);
			}*/
			if (_drawScore)
			{
				char label[32];
				sprintf(label, "score=%.2f", r.score);
				cv::putText(dimg, label, cv::Point(r.roi.x + r.roi.width / 2, r.roi.y + r.roi.height / 2), cv::FONT_HERSHEY_PLAIN, 1.5, Scalar(255, 0, 0), 2, CV_AA);
			}
		}
		return dimg;
	}

	Mat renderResults(const cv::Mat &img, FrameData &fd, ModelSet &ms, bool _drawContour , bool _drawBlend , bool _drawFrame, bool _drawScore)
	{
		cv::Mat dimg = img.clone();

		for (auto &r : fd.objs)
		{
			if (r.score == 0.f /*|| r.roi.empty()*/)
				continue;

			auto pose = r.pose.get<std::vector<RigidPose>>().front();

			CVRMats mats;
			mats.mModel =  cvrm::fromR33T(pose.R, pose.t);
			mats.mProjection = cvrm::fromK(fd.cameraK, img.size(), 0.1, 3000);

			//cout << mats.modelView() << endl;

			auto modelPtr = ms.getModel(r.modelIndex);
			
		//	mats = CVRMats(modelPtr->get3D(), img.size());
		//	cout << mats.modelView() << endl;

			if (_drawContour || _drawBlend)
			{
				CVRModel &m3d = modelPtr->get3DModel();
				CVRender render(m3d);
				//auto rr = render.exec(mats, img.size(), CVRM_IMAGE | CVRM_DEPTH, CVRM_DEFAULT, nullptr, r.roi);
				auto rr = render.exec(mats, img.size(), CVRM_IMAGE | CVRM_DEPTH, CVRM_DEFAULT, nullptr);
				//cv::imshow("rr", rr.img);
				Mat1b mask = getRenderMask(rr.depth);
				Rect roi = cv::get_mask_roi(DWHS(mask), 127);

				if (roi.empty())
					continue;
				
				if (_drawBlend)
				{
					Mat t;
					cv::addWeighted(dimg(roi), 0.5, rr.img(roi), 0.5, 0, t);
					t.copyTo(dimg(roi), mask(roi));
				}
				if (_drawContour)
				{
					std::vector<std::vector<Point> > cont;
					cv::findContours(mask, cont, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
					drawContours(dimg, cont, -1, Scalar(255, 0, 0), 2, CV_AA);
				}
			}
			/*if (_drawFrame)
			{
				auto mi = modelPtr->getObject<re3d::MeshInfo>();

				cv::Point3f oxyz[4];
				mi->getFrameR0(oxyz);

				CVRProjector prj(mats, img.size());
				cv::Point2f oxyzPrj[4];
				prj.project(oxyz, oxyzPrj, 4);

				drawFrame(dimg, oxyzPrj);
			}*/
			if (_drawScore)
			{
				char label[32];
				sprintf(label, "score=%.2f", r.score);
				cv::putText(dimg, label, cv::Point(r.roi.x + r.roi.width / 2, r.roi.y + r.roi.height / 2), cv::FONT_HERSHEY_PLAIN, 1.5, Scalar(255, 0, 0), 2, CV_AA);
			}
		}
		return dimg;
	}

}
