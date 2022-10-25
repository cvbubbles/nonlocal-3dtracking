#pragma once

#include"CVRender\cvrender.h"
using namespace cv;

class CVXShowMatch
	:public CVRShowModelBase
{
public:
	cv::CVWindow *wnd;
public:
	int       _modelImgWidth;
	cv::Mat   _camImg;
	std::vector<Point3f>  _modelPts;
	std::vector<Point2f>  _camPts;
	std::vector<Scalar>   _ptColors;
	cv::Rect  _camROI;

	std::vector<Point3f>  _modelPtsProj;
	cv::Point _pt1;
	std::vector<int>  _vsel;

	bool _inROI(const Point2f &p)
	{
		int x = int(p.x + 0.5), y = int(p.y + 0.5);
		return x >= _camROI.x&&x < _camROI.x + _camROI.width && y >= _camROI.y && y < _camROI.y + _camROI.height;
	}

	Mat _composite(const CVRResult &result)
	{
		Mat3b modelImg = result.img;

		CV_Assert(modelImg.rows == _camImg.rows);
		cv::Mat3b dimg(_camImg.rows, _camImg.cols + modelImg.cols);
		 
		copyMem(modelImg, dimg(Rect(0, 0, modelImg.cols, modelImg.rows)));
		copyMem(_camImg, dimg(Rect(modelImg.cols, 0, _camImg.cols, _camImg.rows)));

		Mat1f depth = result.depth;

		_modelPtsProj.resize(_modelPts.size());

		for (size_t i = 0; i < _modelPts.size(); ++i)
		{
			Point2f q = _camPts[i];
			if (!_inROI(q))
				continue;

			Point3f p = result.project(_modelPts[i]);
			_modelPtsProj[i] = p;

			int px = int(p.x + 0.5), py = int(p.y + 0.5);
			if (uint(px) < depth.cols && uint(py) < depth.rows)
			{
				if(p.z-depth(py,px)<1e-3f)
					cv::line(dimg, cv::Point(int(p.x + 0.5), int(p.y + 0.5)), cv::Point(int(q.x + 0.5) + modelImg.cols, int(q.y + 0.5)), _ptColors[i], 1, CV_AA);
			}
		}
		for (auto i : _vsel)
		{
			if (uint(i) < _modelPts.size())
			{
				cv::Point pt1 = Point(int(_modelPtsProj[i].x + 0.5), int(_modelPtsProj[i].y + 0.5)), pt2 = Point(int(_camPts[i].x + 0.5) + _modelImgWidth, int(_camPts[i].y + 0.5));

				cv::line(dimg, pt1, pt2, _ptColors[i], 1, CV_AA);
				cv::circle(dimg, pt1, 3, _ptColors[i], 2, CV_AA);
				cv::circle(dimg, pt2, 3, _ptColors[i], 2, CV_AA);
			}
		}

		return dimg;
	}

public:
	CVXShowMatch(cv::CVWindow *_wnd, const CVRModel &_model, Size _viewSize, const CVRMats &_mats, const cv::Mat &_bgImg, int _renderFlags)
		:wnd(_wnd), CVRShowModelBase(_model, _viewSize, _mats, _bgImg, _renderFlags)
	{}

	void set(const cv::Mat &camImg, const std::vector<Point3f> &modelPts, const std::vector<Point2f> &camPts)
	{
		_camImg = camImg.clone();
		_modelPts = modelPts;
		_camPts = camPts;

		_ptColors.resize(_modelPts.size());
		for (size_t i = 0; i < _ptColors.size(); ++i)
		{
			_ptColors[i] = Scalar(rand() % 255, rand() % 255, rand() % 255);
		}
		_camROI = Rect(0, 0, _camImg.cols, _camImg.rows);
		_modelImgWidth = _camImg.rows;
	}

	void setROI(cv::Point pt1, cv::Point pt2)
	{
		_camROI = Rect(__min(pt1.x, pt2.x) - _modelImgWidth, __min(pt1.y, pt2.y), abs(pt1.x - pt2.x), abs(pt1.y - pt2.y));
		this->present(this->currentResult);
	}
	void selectActive(cv::Point pt)
	{
		int mi = -1;
		float md = FLT_MAX;
		for (int i=0; i<(int)_modelPtsProj.size(); ++i)
		{
			Vec2f dv = Vec2f(_modelPtsProj[i].x, _modelPtsProj[i].y) - Vec2f(pt.x, pt.y);
			float d = dv.dot(dv);
			if (d < md)
				mi = i, md = d;
		}
		if (mi >= 0)
		{
			_vsel.clear();
			_vsel.push_back(mi);
			this->present(this->currentResult);
		}
	}
	void onEvent(int evt, int param1, int param2, cv::CVEventData data)
	{
		if (evt == cv::EVENT_LBUTTONDOWN)
			_pt1.x = param1, _pt1.y = param2;
		else if (evt == cv::EVENT_LBUTTONUP)
		{
			if (_pt1.x > _modelImgWidth)
				this->setROI(_pt1, Point(param1, param2));
			else
				this->selectActive(Point(param1, param2));
		}
	}

	virtual void present(const CVRResult &result)
	{
		if (wnd)
			wnd->show(_composite(result));
	}
};

typedef std::shared_ptr<CVXShowMatch> CVXShowMatchPtr;

inline CVXShowMatchPtr showModelMatches(const std::string &wndName, const CVRModel &model, const std::vector<Point3f> &modelPts, const cv::Mat &img, const std::vector<Point2f> &imgPts)
{
	cv::CVWindow *wnd = cv::getWindow(wndName, true);
	CVXShowMatchPtr dptr;
	if (wnd)
	{
		Size viewSize(img.rows, img.rows);
		//realize the window
		cv::Mat1b timg = cv::Mat1b::zeros(viewSize);
		wnd->show(timg);

		CVRMats mats(model, viewSize);

		dptr = CVXShowMatchPtr(new CVXShowMatch(wnd, model, viewSize, mats, Mat(), CVRM_DEFAULT));
		dptr->set(img, modelPts, imgPts);

		wnd->setEventHandler([dptr](int evt, int param1, int param2, cv::CVEventData data) mutable {
			if (data.ival&cv::EVENT_FLAG_CTRLKEY)
				dptr->onEvent(evt, param1, param2, data);
			else
				_postShowModelEvent(_CVRShowModelEvent(dptr.get(), evt, param1, param2, data.ival, _CVRShowModelEvent::F_IGNORABLE));

		}, "cvxShowModelMatchesEventHandler");

		dptr->update(false);
	}
	return dptr;
}


