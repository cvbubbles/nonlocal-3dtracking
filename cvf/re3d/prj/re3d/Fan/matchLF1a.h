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

	if (desc.type() != CV_32FC1)
	{
		desc = desc;
	}

}
Mat1b getRenderMask(const Mat3b &renderImg)
{
	Mat1b mask = Mat1b::zeros(renderImg.size());
	for_each_2(DWHNC(renderImg), DN1(mask), [](const uchar *p, uchar &m) {
		m = p[0] == 0 && p[1] == 0 && p[2] == 0 ? 0 : 255;
	});
	return mask;
}

struct DView
{
	CVRResult rr;

	std::vector<KeyPoint> kp;
	std::vector<Point3f>  kp3;
	Mat desc;
public:
	void detect()
	{
		CV_Assert(rr.img.type() == CV_8UC3);
		::detect(rr.img, kp, desc, getRenderMask(rr.img));

		kp3.resize(kp.size());
		for (size_t i = 0; i < kp.size(); ++i)
		{
			kp3[i] = rr.unproject(kp[i].pt.x, kp[i].pt.y);
		}
	}
};

void renderViews(const CVRModel &model, std::vector<DView> &views, int nViews, Size viewSize)
{
	CVRender render(model);
	CVRMats objMats(model, viewSize);


	std::vector<Vec3f> vecs;
	cvrm::sampleSphere(vecs, nViews);

	views.clear();
	views.resize(nViews);
	for (int i = 0; i < nViews; ++i)
	{
		objMats.mModel = cvrm::rotate(vecs[i], Vec3f(0, 0, 1));
		views[i].rr = render.exec(objMats, viewSize);
	}
}

struct DPoint
{
	Point3f pt;
	float density;
};

void getPoints(const std::vector<DView> &views, std::vector<DPoint> &pts, Mat &desc)
{
	size_t npt = 0;
	for (auto &v : views)
		npt += v.kp.size();

	pts.resize(npt);

	desc.create(npt, views.front().desc.cols, views.front().desc.type());
	int descRowSize = desc.elemSize()*desc.cols;

	size_t k = 0;
	for (auto &v : views)
	{
		for (int i = 0; i < v.kp.size(); ++i)
		{
			auto &px(pts[k]);
			px.pt = v.kp3[i];
			memcpy(desc.ptr(k), v.desc.ptr(i), descRowSize);
			++k;
		}
	}
}

float getd(const std::vector<DPoint> &pts, int D = 300)
{
	Vec3f vmin = pts[0].pt, vmax = vmin;
	for (auto &p : pts)
	{
		Vec3f v = p.pt;
		for (int i = 0; i < 3; ++i)
		{
			if (v[i] < vmin[i])
				vmin[i] = v[i];
			else if (v[i]>vmax[i])
				vmax[i] = v[i];
		}
	}
	float d = 0;
	for (int i = 0; i < 3; ++i)
	{
		float r = (vmax[i] - vmin[i]) / D;
		d += r*r;
	}
	return sqrt(d);
}

void selPoints(std::vector<DPoint> &pts, std::vector<DPoint> &dsel, int N)
{
	size_t npt = pts.size();

	for (size_t i = 0; i < npt; ++i)
		pts[i].density = 0;

	float r = getd(pts, 1000);
	r = r*r;

	for (size_t i = 0; i < npt; ++i)
	{
		auto &p(pts[i]);
		for (size_t j = i + 1; j < npt; ++j)
		{
			Point3f dv = p.pt - pts[j].pt;
			if (dv.dot(dv) < r)
			{
				p.density += 1;
				pts[j].density += 1;
			}
		}
	}
	std::sort(pts.begin(), pts.end(), [](const DPoint &a, const DPoint &b) {
		return a.density > b.density;
		//return a.density < b.density;
	});
	std::vector<char> mask(pts.size(), 0);
	dsel.clear();
	dsel.reserve(N);
	for (size_t i = 0; i < npt; ++i)
	{
		if (mask[i] == 0)
		{
			for (size_t j = i + 1; j < npt; ++j)
			{
				Point3f dv = pts[i].pt - pts[j].pt;
				if (dv.dot(dv) < r)
					mask[j] = 1;
			}
			mask[i] = 1;
			dsel.push_back(pts[i]);
			if (dsel.size() >= N)
				break;
		}
	}
}

//#include"FLANN/flann.h"

void selPoints(const std::vector<DPoint> &pts, const Mat &ptsDesc, std::vector<DPoint> &dsel, Mat &selDesc, int K = 20)
{
	flann::Index  knn;
	//knn.build(ptsDesc, flann::LshIndexParams(20,10,0),cvflann::FLANN_DIST_HAMMING);
	knn.build(ptsDesc, flann::KDTreeIndexParams(), cvflann::FLANN_DIST_L2);

	Mat1i indices;
	Mat1f dists;
	knn.knnSearch(ptsDesc, indices, dists, K);

	float dmax = getd(pts, 300);
	dmax *= dmax;

	struct DPt
	{
		int id;
		float nm;
	};
	std::vector<DPt>  dpt(pts.size());
	for (int i = 0; i < (int)pts.size(); ++i)
	{
		const int *vi = indices.ptr<int>(i);
		int nm = 0;
		for (int j = 0; j < K; ++j)
		{
			Vec3f dv = pts[i].pt - pts[vi[j]].pt;
			if (dv.dot(dv) < dmax)
				++nm;
		}
		dpt[i] = { i,(float)nm };
	}
	std::sort(dpt.begin(), dpt.end(), [](const DPt &a, const DPt &b) {
		return a.nm > b.nm;
	});

	dsel.clear();

#if 0
	for (auto &p : dpt)
		if (p.nm > K / 2)
			dsel.push_back(pts[p.id]);
#else
	std::vector<char>  mask(dpt.size(), 0);

	CV_Assert(ptsDesc.type() == CV_32FC1);
	std::unique_ptr<float[]> _vdesc(new float[ptsDesc.rows*ptsDesc.cols]);
	float *vdesc = _vdesc.get();
	memset(vdesc, 0, sizeof(float)*ptsDesc.rows*ptsDesc.cols);

	for (int i = 0; i < (int)dpt.size(); ++i)
	{
		if (dpt[i].nm < K*0.5f)
			break;

		int pid = dpt[i].id;
		if (mask[pid] == 0)
		{
			const int *vi = indices.ptr<int>(pid);
			for (int j = 0; j < K; ++j)
			{
				Vec3f dv = pts[pid].pt - pts[vi[j]].pt;
				if (dv.dot(dv) < dmax)
					mask[vi[j]] = 1;
			}
			dsel.push_back(pts[pid]);

			memcpy(vdesc, ptsDesc.ptr(pid), sizeof(float)*ptsDesc.cols);
			vdesc += ptsDesc.cols;
		}
	}
#endif

	selDesc.create(dsel.size(), ptsDesc.cols, ptsDesc.type());
	memcpy(selDesc.data, _vdesc.get(), selDesc.rows*selDesc.cols * sizeof(float));
}

CVXShowModelPtr showModelPoints(const std::string &wndName, const CVRModel &model, const std::vector<DPoint> &pts)
{
	std::vector<Point3f> pts3(pts.size());
	for (size_t i = 0; i < pts.size(); ++i)
		pts3[i] = pts[i].pt;

	auto wd = mdshow(wndName, model);
	wd->userDraw = newUserDraw([pts3]() {
		drawPoints(&pts3[0], pts3.size(), 2);
	});
	wd->update(false);
	return wd;
}

void saveModelData(const std::string &file, const std::vector<DPoint> &vpt, const Mat &desc)
{
	CV_Assert(desc.type() == CV_32FC1);
	Mat data(desc.rows, desc.cols + 3, CV_32FC1);
	for (int i = 0; i < vpt.size(); ++i)
	{
		float *dp = data.ptr<float>(i);
		memcpy(dp, &vpt[i].pt, sizeof(float) * 3);
		memcpy(dp + 3, desc.ptr<float>(i), sizeof(float)*desc.cols);
	}
	saveMatData(file, data);
}

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

void proModel(std::string dataDir, std::string outDir)
{
	std::string file = dataDir + "/3d/car.3ds";

	CVRModel model(file);
	Size viewSize(500, 500);
	std::vector<DView> views;
	renderViews(model, views, 300, viewSize);

	atomic_int n = 0;
#pragma omp parallel for num_threads(8) 
	for (int i = 0; i < (int)views.size(); ++i)
	{
		views[i].detect();
		printf("%d/%d\n", n++, views.size());
	}

	std::vector<DPoint> pts;
	Mat ptsDesc;
	getPoints(views, pts, ptsDesc);
	views.clear();

	printf("npt=%d\n", (int)pts.size());

	showModelPoints("model", model, pts);

	std::vector<DPoint> ptSel;
	Mat selDesc;
	selPoints(pts, ptsDesc, ptSel, selDesc);

	printf("nsel=%d\n", (int)ptSel.size());

	std::string dfile = outDir + ff::GetFileName(file, false) + ".md.akaze";
	saveModelData(dfile, ptSel, selDesc);

	showModelPoints("sel", model, ptSel);

	cv::waitKeyEx();
}


//===============================================================================================

void findMatch(const Mat &camImgDesc, const Mat &objImgDesc, std::vector<DMatch> &matches)
{
	Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("FlannBased");
	//FlannBasedMatcher matcher;

	std::vector<DMatch> objMatches, camMatches;
	//matcher->match(objImgDesc, camImgDesc, objMatches);
	matcher->match(camImgDesc, objImgDesc, camMatches);
	matches.swap(camMatches);

#if 0
	std::vector<int> nback(camImgDesc.rows, 0);
	for (auto &m : objMatches)
		nback[m.trainIdx]++;

	matches.clear();
	for (size_t i = 0; i < objMatches.size(); ++i)
		if (objMatches[i].queryIdx == camMatches[objMatches[i].trainIdx].trainIdx)
			//if (nback[objMatches[i].trainIdx] <= 3)
			matches.push_back(objMatches[i]);
#endif
}

template<typename _PtT>
Mat1i findPointKNN(std::vector<_PtT> &vpt, int K)
{
	flann::Index  knn;
	Mat1f m(vpt.size(), sizeof(_PtT)/sizeof(float), (float*)&vpt[0]);
	knn.build(m, flann::KDTreeIndexParams());

	Mat indices;
	Mat dists;
	knn.knnSearch(m, indices, dists, K);

	return indices;
}

void filterOutlierLPM(std::vector<Point2f> &imgPoints, std::vector<Point3f> &objPoints)
{
	const int K = 10, T=6;
	Mat1i imgNN = findPointKNN(imgPoints, K);
	Mat1i objNN = findPointKNN(objPoints, K);

	std::vector<int>  vtag(imgPoints.size(), 0);
	std::vector<int>  vscore(imgPoints.size());
	int tag = 0;
	for (int i = 0; i < (int)imgPoints.size(); ++i)
	{
		const int *k1 = imgNN.ptr<int>(i);
		const int *k2 = objNN.ptr<int>(i);

		int n = 0;


		++tag;
		for (int j = 0; j < K; ++j)
			vtag[k1[j]] = tag;
		for (int j = 0; j < K; ++j)
			if (vtag[k2[j]] == tag)
				++n;

		++tag;
		for (int j = 0; j < K; ++j)
			vtag[k2[j]] = tag;
		for (int j = 0; j < K; ++j)
			if (vtag[k1[j]] == tag)
				++n;

		vscore[i] = n;
	}
	//std::sort(vscore.begin(), vscore.end());
	int nsel = 0;
	for(int i=0; i<(int)imgPoints.size(); ++i)
		if (vscore[i] > T)
		{
			imgPoints[nsel] = imgPoints[i];
			objPoints[nsel] = objPoints[i];
			++nsel;
		}
	
	printf("nsel=%d/%d\n", nsel, (int)imgPoints.size());

	imgPoints.resize(nsel);
	objPoints.resize(nsel);
}

void getPointPairs(const Mat3b &camImg, const Mat &objImgDesc, const std::vector<Point3f> &objKP,
	std::vector<Point2f> &imgPoints, std::vector<Point3f> &objPoints)
{
	time_t beg = clock();

	Mat camImgDesc;
	std::vector<KeyPoint> camImgKP;
	detect(camImg, camImgKP, camImgDesc);

	printf("t-detect=%d\n", int(clock() - beg)); beg = clock();

	std::vector<DMatch> matches;
	//matcher->match(objImgDesc, camImgDesc, matches);
	findMatch(camImgDesc, objImgDesc, matches);

	std::sort(matches.begin(), matches.end(), [](const DMatch &a, const DMatch &b) {
		return a.distance < b.distance;
	});
	//matches.resize(matches.size() / 2);
	matches.resize(100);

	imgPoints.resize(matches.size());
	objPoints.resize(matches.size());

	for (size_t i = 0; i < matches.size(); ++i)
	{
		auto &m(matches[i]);
		imgPoints[i] = camImgKP[m.queryIdx].pt;
		objPoints[i] = objKP[m.trainIdx];
	}

	//filterOutlierLPM(imgPoints, objPoints);
	printf("t-match=%d\n", int(clock() - beg));
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
	/*= cv::Mat(1, 5, CV_64F);
	distCoeffs.at<double>(0, 0) = 0.178265644047564;
	distCoeffs.at<double>(0, 1) = -0.1942515244450349;
	distCoeffs.at<double>(0, 2) = -0.001475393656445968;
	distCoeffs.at<double>(0, 3) = -0.0138682465167786;
	distCoeffs.at<double>(0, 4) = -2.310032357752005;*/

	CVRMats mats;

	Mat rvec, tvec;
	if (!cv::solvePnPRansac(objPts, imgPts, K, distCoeffs, rvec, tvec, false,1000,8,0.99,noArray(), SOLVEPNP_AP3P))
	//	if(!cv::solvePnP(objPts, imgPts, K, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_EPNP))
		return mats;

	mats.mModel = cvrm::fromRT(rvec, tvec);
	mats.mProjection = cvrm::fromK(K, viewSize, 0.1, 2000);

	return mats;
}

void matchModel(std::string dataDir, std::string outDir)
{
	std::string name = "box1";
	Mat3b img = imread(dataDir + name + ".png");
	std::vector<Point3f> objKP;
	Mat objDesc;
	loadModelData(outDir + name + ".md.akaze", objKP, objDesc);

	
	std::vector<Point2f> imgPt;
	std::vector<Point3f> objPt;
	getPointPairs(img, objDesc, objKP, imgPt, objPt);

	time_t beg = clock();
	CVRMats mats = solveObjPose(imgPt, objPt, img.size());
	printf("solvePose Time=%d\n", int(clock() - beg));

	CVRModel model(dataDir + "/3d/" + name + ".3ds");
	CVRender render(model);
	render.setBgImage(img);

	CVRResult result = render.exec(mats, img.size(), -1, CVRM_ENABLE_ALL);
	imshow("result", result.img);

	showModelMatches("matches", model, objPt, img, imgPt);

	cvxWaitKey();
}

_CMDI_END

