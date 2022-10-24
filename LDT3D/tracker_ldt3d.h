#pragma once
#include"CVX/core.h"
#include"CVRender/cvrender.h"
using namespace cv;
//#include"videoseg.h"
#include"BFC/stdf.h"
#include"BFC/portable.h"
#include"BFC/bfstream.h"
#include"CVX/bfsio.h"
#include"CVX/vis.h"
#include<queue>
#include"tracker_base.h"
#include"utils.h"

_TRACKER_BEG(v1)

inline Mat1b getRenderMask(const Mat1f& depth, float eps = 1e-6)
{
	Mat1b mask = Mat1b::zeros(depth.size());
	for_each_2(DWHN1(depth), DN1(mask), [eps](float d, uchar& m) {
		m = fabs(1.0f - d) < eps ? 0 : 255;
		});
	return mask;
}


template<typename _Tp>
inline Rect_<_Tp> _getBoundingBox2D(const std::vector<Point_<_Tp>>& pts)
{
	_Tp L = INT_MAX, T = INT_MAX, R = 0, B = 0;
	for (auto& p : pts)
	{
		if (p.x < L)
			L = p.x;
		if (p.x > R)
			R = p.x;
		if (p.y < T)
			T = p.y;
		if (p.y > B)
			B = p.y;
	}
	return Rect_<_Tp>(L, T, R - L, B - T);
}

inline Rect getBoundingBox2D(const std::vector<Point>& pts)
{
	return _getBoundingBox2D(pts);
}
inline Rect_<float> getBoundingBox2D(const std::vector<Point2f>& pts)
{
	return _getBoundingBox2D(pts);
}

struct CPoint
{
	Point3f    center;
	Point3f    normal;

	DEFINE_BFS_IO_2(CPoint, center, normal)
};



class EdgeSampler
{
public:
	static void _sample(Rect roiRect, const Mat1f& depth, CVRProjector& prj, std::vector<CPoint>& c3d, int nSamples)
	{
		Mat1b depthMask = getRenderMask(depth);

		std::vector<std::vector<cv::Point>> contours;
		cv::findContours(depthMask, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

		const int dstride = stepC(depth);
		auto get3D = [&depthMask, &depth, dstride, &prj, roiRect](int x, int y, Point3f& P, float& _z) {
			float z = 0;
			if (depthMask(y, x) != 0)
				z = depth(y, x);
			else
			{
				CV_Assert(false);
			}
			P = prj.unproject(float(x + roiRect.x), float(y + roiRect.y), z);
			_z = z;

			return true;
		};

		size_t npt = 0;
		for (auto& c : contours)
			npt += c.size();
		int nSkip = (int)npt / nSamples;

		const int smoothWSZ = 15, hwsz = smoothWSZ / 2;
		float maxDiff = 1.f;

		for (auto& c : contours)
		{
			double area = cv::contourArea(c, true);
			if (area < 0) //if is clockwise
				std::reverse(c.begin(), c.end()); //make it counter-clockwise

			Mat2i  cimg(1, c.size(), (Vec2i*)&c[0], int(c.size()) * sizeof(Vec2f));
			Mat2f  smoothed;
			boxFilter(cimg, smoothed, CV_32F, Size(smoothWSZ, 1));
			const Point2f* smoothedPts = smoothed.ptr<Point2f>();

			for (int i = nSkip / 2; i < (int)c.size(); i += nSkip)
			{
				CPoint P;
				float depth;
				if (get3D(c[i].x, c[i].y, P.center, depth))
				{
					Point2f n(0, 0);
					if (i - hwsz >= 0)
						n += smoothedPts[i] - smoothedPts[i - hwsz];
					if (i + hwsz < smoothed.cols)
						n += smoothedPts[i + hwsz] - smoothedPts[i];
					n = normalize(Vec2f(n));
					n = Point2f(-n.y, n.x);
					Point2f q = Point2f(c[i]) + n + Point2f(roiRect.x, roiRect.y);
					P.normal = prj.unproject(q.x, q.y, depth);
					c3d.push_back(P);
				}
			}
		}
	}

	static void sample(std::vector<CPoint>& c3d, const CVRResult& rr, int nSamples, Rect roiRect = Rect(0, 0, 0, 0), Size imgSize = Size(0, 0))
	{
		//cout << nSamples << endl;
		if (imgSize.width == 0 || imgSize.height == 0)
			imgSize = rr.img.size();

		if (roiRect.width == 0 || roiRect.height == 0)
			roiRect = Rect(0, 0, imgSize.width, imgSize.height);

		CVRProjector prj(rr.mats, imgSize);

		_sample(roiRect, rr.depth, prj, c3d, nSamples);
	}
};


struct Projector
{
	Matx33f _KR;
	Vec3f _Kt;
public:
	Projector(const Matx33f& _K, const Matx33f& _R, const Vec3f& _t)
		:_KR(_K* _R), _Kt(_K* _t)
	{
	}
	Point2f operator()(const Point3f& P) const
	{
		Vec3f p = _KR * Vec3f(P) + _Kt;
		return Point2f(p[0] / p[2], p[1] / p[2]);
	}
	template<typename _ValT, typename _getPointT>
	std::vector<Point2f> operator()(const std::vector<_ValT>& vP, _getPointT getPoint = [](const _ValT& v) {return v; }) const
	{
		std::vector<Point2f> vp(vP.size());
		for (int i = 0; i < (int)vP.size(); ++i)
			vp[i] = (*this)(getPoint(vP[i]));
		return vp;
	}
};


struct DView
{
	Vec3f       viewDir;
	cv::Matx33f R;
	
	std::vector<CPoint>  contourPoints3d;

	DEFINE_BFS_IO_3(DView, viewDir, R, contourPoints3d)
};

struct ViewIndex
{
	Mat1i  _uvIndex;
	Mat1i  _knnNbrs;
	double _du, _dv;
public:
	static void dir2uv(const Vec3f& dir, double& u, double& v)
	{
		v = acos(dir[2]);
		u = atan2(dir[1], dir[0]);
		if (u < 0)
			u += CV_PI * 2;
	}
	static Vec3f uv2dir(double u, double v)
	{
		return Vec3f(cos(u) * sin(v), sin(u) * sin(v), cos(v));
	}
	const int* getKnn(int vi) const
	{
		return _knnNbrs.ptr<int>(vi) + 1;
	}
	int getK() const
	{
		return _knnNbrs.cols - 1;
	}
	void build(const std::vector<DView>& views, int nU = 200, int nV = 100, int k = 5)
	{
		Mat1f viewDirs(Size(3, views.size()));
		for (size_t i = 0; i < views.size(); ++i)
		{
			memcpy(viewDirs.ptr(i), &views[i].viewDir, sizeof(float) * 3);
		}

		flann::Index  knn;
		knn.build(viewDirs, flann::KDTreeIndexParams(), cvflann::FLANN_DIST_L2);

		Mat1f dists;
		flann::SearchParams param;

		{
			Mat1i indices;
			knn.knnSearch(viewDirs, indices, dists, k + 1);

			CV_Assert(indices.size() == Size(k + 1, views.size()));
			for (int i = 0; i < (int)views.size(); ++i)
			{
				CV_Assert(indices.ptr<int>(i)[0] == i);
			}
			this->_knnNbrs = indices;
		}
		{
			Mat1f uvDir(Size(3, nU * nV));

			double du = 2 * CV_PI / (nU - 1), dv = CV_PI / (nV - 1);
			_du = du;
			_dv = dv;

			for (int vi = 0; vi < nV; ++vi)
			{
				for (int ui = 0; ui < nU; ++ui)
				{
					double u = ui * du, v = vi * dv;
					Vec3f dir = uv2dir(u, v);
					memcpy(uvDir.ptr(vi * nU + ui), &dir, sizeof(float) * 3);
				}
			}

			Mat1i indices;
			knn.knnSearch(uvDir, indices, dists, 1);
			CV_Assert(indices.size() == Size(1, nU * nV));


			this->_uvIndex = indices.reshape(1, nV);
		}
	}
	int getViewInDir(const Vec3f& dir) const
	{
		CV_Assert(fabs(dir.dot(dir) - 1) < 1e-2f);

		double u, v;
		dir2uv(dir, u, v);
		int ui = int(u / _du + 0.5), vi = int(v / _dv + 0.5);
		//return _views[_uvIndex(vi, ui)].c3d;
		return _uvIndex(vi, ui);
	}
};


class DFRHandler
{
public:
	virtual int test(const Matx33f& R, const Vec3f& t) = 0;

	virtual ~DFRHandler() {}
};

static UpdateHandler* g_uhdl = nullptr;
static int g_totalUpdates = 0;


struct FirstStageOptimizer
{
public:
	struct ContourPoint
	{
		float   w; //weight
		float   x; //position on the scan-line
	};
	enum { MAX_POINTS_PER_LINE_FIRST = 3};
	
	struct ScanLine
	{
		float     y;
		Point2f   xdir;
		Point2f   xstart;
		ContourPoint  vPoints[MAX_POINTS_PER_LINE_FIRST];
		int       nPoints;
		short*    cpIndex; //index of the closest contour point for each x position
	public:
		void setCoordinates(const Point2f& start, const Point2f& end, float y)
		{
			this->xstart = start;
			xdir = (Point2f)normalize(Vec2f(end - start));
			this->y = y;
		}
		int getClosestContourPoint(const Point2f& pt, int xsize)
		{
			int x = int((pt - xstart).dot(xdir) + 0.5f);
			if (uint(x) < uint(xsize))
				return cpIndex[x];
			return -1;
		}
		
	};

	struct DirData
	{
		Vec2f      dir;
		Point2f    ystart;
		Point2f    ydir;
		std::vector<ScanLine>  lines;
		Mat1s         _cpIndexBuf;
	public:
		void setCoordinates(const Point2f& ystart, const Point2f& ypt)
		{
			this->ystart = ystart;
			ydir = (Point2f)normalize(Vec2f(ypt - ystart));
		}
		void resize(int rows, int cols)
		{
			lines.clear();
			lines.resize(rows);
			_cpIndexBuf.create(rows, cols);
			for (int y = 0; y < rows; ++y)
			{
				lines[y].cpIndex = _cpIndexBuf.ptr<short>(y);
			}
		}
		const ScanLine* getScanLine(const Point2f& pt, int& matchedContourPoint)
		{
			int y = int((pt - ystart).dot(ydir) + 0.5f);
			if (uint(y) >= lines.size())
				return nullptr;
			matchedContourPoint = lines[y].getClosestContourPoint(pt, int(_cpIndexBuf.cols));
			return &lines[y];
		}
	};

	std::vector<DirData>  _dirs;
	Rect  _roi;
public:
	static void _gaussianFitting(const float* data, int size, ContourPoint& cp)
	{
		float w = 0.f, wsum = 0.f;
		for (int i = 0; i < size; ++i)
		{
			wsum += data[i] * float(i);
			w += data[i];
		}

		cp.x = wsum/w;
	}
	struct _LineBuilder
	{
		struct LocalMaxima
		{
			int x;
			float val;
		};
		std::vector<LocalMaxima>  _lmBuf;
	public:
		_LineBuilder(int size)
		{
			_lmBuf.resize(size);
		}

		void operator()(ScanLine& line, const float* data, int size, int gaussWindowSizeHalf)
		{
			LocalMaxima* vlm = &_lmBuf[0];
			int nlm = 0;
			for (int i = 1; i < size - 1; ++i)
			{
				if (data[i] > data[i - 1] && data[i] > data[i + 1])
				{
					auto& lm = vlm[nlm++];
					lm.x = i;
					lm.val = data[i];
				}
			}
			int max_nlm = nlm;
			
			if (nlm > MAX_POINTS_PER_LINE_FIRST)
			{
				std::sort(vlm, vlm + nlm, [](const LocalMaxima& a, const LocalMaxima& b) {
					return a.val > b.val;
					});
				nlm = MAX_POINTS_PER_LINE_FIRST;

				std::sort(vlm, vlm + nlm, [](const LocalMaxima& a, const LocalMaxima& b) {
					return a.x < b.x;
					});
			}
			for (int i = 0; i < nlm; ++i)
			{
				auto& lm = vlm[i];
				auto& cp = line.vPoints[i];

				const int start = __max(0, lm.x - gaussWindowSizeHalf), end = __min(size, lm.x + gaussWindowSizeHalf);
				_gaussianFitting(data + start, end - start, cp);

				cp.x += (float)start;
				cp.w = lm.val;
				//cp.w = new_alpha * lm.val;
			}
			line.nPoints = nlm;

			if (nlm <= 1)
				memset(line.cpIndex, nlm == 0 ? 0xFF : 0, sizeof(short) * size);
			else
			{
				
				int start = 0;
				for (int pi = 0; pi < nlm - 1; ++pi)
				{
					int end= int(int(line.vPoints[pi].x + line.vPoints[pi+1].x) / 2 + 0.5f)+1;
					for (int i = start; i < end; ++i)
						line.cpIndex[i] = pi;
					start = end;
				}
				for (int i = start; i < size; ++i)
					line.cpIndex[i] = nlm - 1;
			}
		}
	};
	static void _calcScanLinesForRows(const Mat1f& prob, DirData& dirPositive, DirData& dirNegative, const Matx23f& invA)
	{
		const int gaussWindowSizeHalf = 3;

		Mat1f edgeProb;
		cv::Sobel(prob, edgeProb, CV_32F, 1, 0, 7);


		{
			Point2f O = transA(Point2f(0.f, 0.f), invA.val), P = transA(Point2f(0.f, float(prob.rows - 1)), invA.val);
			dirPositive.setCoordinates(O, P);
			dirNegative.setCoordinates(P, O);
		}
		dirPositive.resize(prob.rows, prob.cols);
		dirNegative.resize(prob.rows, prob.cols);

		std::unique_ptr<float[]> _rdata(new float[prob.cols * 2]);
		float* posData = _rdata.get(), * negData = posData + prob.cols;
		_LineBuilder buildLine(prob.cols);

		const int xend = int(prob.cols - 1);
		for (int y = 0; y < prob.rows; ++y)
		{
			auto& positiveLine = dirPositive.lines[y];
			auto& negativeLine = dirNegative.lines[prob.rows - 1 - y];

			Point2f O = transA(Point2f(0.f, float(y)), invA.val), P = transA(Point2f(float(prob.cols - 1), float(y)), invA.val);
			positiveLine.setCoordinates(O, P, float(y));
			negativeLine.setCoordinates(P, O, float(prob.rows - 1 - y));

			const float* ep = edgeProb.ptr<float>(y);

			for (int x = 0; x < prob.cols; ++x)
			{
				if (ep[x] > 0)
				{
					posData[x] = ep[x]; negData[xend - x] = 0.f;
				}
				else
				{
					posData[x] = 0.f; negData[xend - x] = -ep[x];
				}
			}

			buildLine(positiveLine, posData, prob.cols, gaussWindowSizeHalf);
			buildLine(negativeLine, negData, prob.cols, gaussWindowSizeHalf);
		}
	}

	//Mat  _prob;
	std::vector<int>  _dirIndex;

	void computeScanLines(const Mat1f& prob_, Rect roi)
	{
		//_prob = prob_.clone(); //save for visualization

		const int N = 8;

		Point2f center(float(roi.x + roi.width / 2), float(roi.y + roi.height / 2));
		Rect_<float> roif(roi);
		std::vector<Point2f>  corners = {
			Point2f(roif.x, roif.y), Point2f(roif.x + roif.width,roif.y), Point2f(roif.x + roif.width,roif.y + roif.height),Point2f(roif.x,roif.y + roif.height)
		};

		struct _DDir
		{
			Vec2f   dir;
			Matx23f A;
		};

		_dirs.clear();
		_dirs.resize(N * 2);

		//for (int i = 0; i < N; ++i)
		cv::parallel_for_(cv::Range(0, N), [&](const cv::Range& r) {
			for (int i = r.start; i < r.end; ++i)
			{
				double theta = 180.0 / N * i;
				Matx23f A = getRotationMatrix2D(center, theta, 1.0);
				std::vector<Point2f> Acorners;
				cv::transform(corners, Acorners, A);
				cv::Rect droi = getBoundingBox2D(Acorners);
				A = Matx23f(1, 0, -droi.x,
					0, 1, -droi.y) * A;

				Mat1f dirProb;
				cv::warpAffine(prob_, dirProb, A, droi.size(), INTER_LINEAR, BORDER_CONSTANT, Scalar(0));

				theta = theta / 180.0 * CV_PI;
				auto dir = Vec2f(cos(theta), sin(theta));

				//imshow("dirProb", dirProb);
				//cv::waitKey();

				auto& positiveDir = _dirs[i];
				auto& negativeDir = _dirs[i + N];

				auto invA = invertAffine(A);
				_calcScanLinesForRows(dirProb, positiveDir, negativeDir, invA);

				positiveDir.dir = dir;
				negativeDir.dir = -dir;
			}
			});

		//normalize weight of contour points
		{
			float wMax = 0;
			for (auto& dir : _dirs)
			{
				for (auto& dirLine : dir.lines)
				{
					for (int i = 0; i < dirLine.nPoints; ++i)
						if (dirLine.vPoints[i].w > wMax)
							wMax = dirLine.vPoints[i].w;
				}
			}

			for (auto& dir : _dirs)
			{
				for (auto& dirLine : dir.lines)
				{
					for (int i = 0; i < dirLine.nPoints; ++i)
						dirLine.vPoints[i].w /= wMax;
				}
			}
		}

		//build index of dirs
		if (_dirIndex.empty())
		{
			_dirIndex.resize(361);
			for (int i = 0; i < (int)_dirIndex.size(); ++i)
			{
				float theta = i * CV_PI / 180.f - CV_PI;
				Vec2f dir(cos(theta), sin(theta));

				float cosMax = -1;
				int jm = -1;
				for (int j = 0; j < (int)_dirs.size(); ++j)
				{
					float vcos = _dirs[j].dir.dot(dir);
					if (vcos > cosMax)
					{
						cosMax = vcos;
						jm = j;
					}
				}
				_dirIndex[i] = jm;
			}
		}

		_roi = roi;
	}
	DirData* getDirData(const Vec2f& ptNormal)
	{
		float theta = atan2(ptNormal[1], ptNormal[0]);
		int i = int((theta + CV_PI) * 180 / CV_PI);
		auto* ddx = uint(i) < _dirIndex.size() ? &_dirs[_dirIndex[i]] : nullptr;
		return ddx;
	}

	struct PoseData
		:public Pose
	{
		int itr = 0;
		DFRHandler* hdl = nullptr;
	};

	float calcError(const PoseData& pose, const Matx33f& K, const std::vector<CPoint>& cpoints, float alpha)
	{
		const Matx33f R = pose.R;
		const Point3f t = pose.t;

		const float fx = K(0, 0), fy = K(1, 1), cx = K(0, 2), cy = K(1, 2);

		auto* vcp = &cpoints[0];
		int npt = (int)cpoints.size();
		float err = 0.f;
		float nerr = 0.f;

		for (int i = 0; i < npt; ++i)
		{
			Point3f Q = R * vcp[i].center + t;
			Point3f q = K * Q;
			{
				const int x = int(q.x / q.z + 0.5), y = int(q.y / q.z + 0.5);
				if (uint(x - _roi.x) >= uint(_roi.width) || uint(y - _roi.y) >= uint(_roi.height))
					continue;

				Point3f qn = K * (R * vcp[i].normal + t);
				Vec2f n(qn.x / qn.z - q.x / q.z, qn.y / qn.z - q.y / q.z);
				n = normalize(n);

				Point2f pt(q.x / q.z, q.y / q.z);

				auto* dd = this->getDirData(n);
				if (!dd)
					continue;
				int cpi;
				auto* dirLine = dd->getScanLine(pt, cpi);
				if (!dirLine || cpi < 0)
					continue;

				auto& cp = dirLine->vPoints[cpi];

				Vec2f nx = dirLine->xdir;

				float du = (pt - dirLine->xstart).dot(nx);

				float w = cp.w * cp.w;
				
				err += pow(fabs(du - cp.x), alpha) * w;

				nerr += w;
			}
		}
		return err / nerr;
	}

	int _update(PoseData& pose, const Matx33f& K, const std::vector<CPoint>& cpoints, float alpha, float eps)
	{
		const Matx33f R = pose.R;
		const Point3f t = pose.t;

		if (pose.hdl && pose.hdl->test(R, t) <= 0)
			return 0;

		++g_totalUpdates;

		const float fx = K(0, 0), fy = K(1, 1);

		auto* vcp = &cpoints[0];
		int npt = (int)cpoints.size();

		Matx66f JJ = Matx66f::zeros();
		Vec6f J(0, 0, 0, 0, 0, 0);
		float w_sum = 0.0f;
		for (int i = 0; i < npt; ++i)
		{
			Point3f Q = R * vcp[i].center + t;
			Point3f q = K * Q;
			//if (q.z == 0.f)
			//	continue;
			//else
			{
				const int x = int(q.x / q.z + 0.5), y = int(q.y / q.z + 0.5);
				if (uint(x - _roi.x) >= uint(_roi.width) || uint(y - _roi.y) >= uint(_roi.height))
					continue;

				Point3f qn = K * (R * vcp[i].normal + t);
				Vec2f n(qn.x / qn.z - q.x / q.z, qn.y / qn.z - q.y / q.z);
				n = normalize(n);

				const float X = Q.x, Y = Q.y, Z = Q.z;
				//      |fx/Z   0   -fx*X/Z^2 |   |a  0   b|
				//dq/dQ = |                     | = |        |
				//		|0    fy/Z  -fy*Y/Z^2 |   |0  c   d|
				//
				const float a = fx / Z, b = -fx * X / (Z * Z), c = fy / Z, d = -fy * Y / (Z * Z);

				Point2f pt(q.x / q.z, q.y / q.z);

				auto* dd = this->getDirData(n);
				if (!dd)
					continue;
				int cpi;
				auto* dirLine = dd->getScanLine(pt, cpi);
				if (!dirLine || cpi < 0)
					continue;

				auto& cp = dirLine->vPoints[cpi];

				Vec2f nx = dirLine->xdir;
				float du = (pt - dirLine->xstart).dot(nx);

				Vec3f n_dq_dQ(nx[0] * a, nx[1] * c, nx[0] * b + nx[1] * d);

				auto dt = n_dq_dQ.t() * R;
				auto dR = vcp[i].center.cross(Vec3f(dt.val[0], dt.val[1], dt.val[2]));

				Vec6f j(dt.val[0], dt.val[1], dt.val[2], dR.x, dR.y, dR.z);
				
				float w = pow(1.f / (fabs(du - cp.x) + 1.f), 2.f-alpha) * cp.w * cp.w;
				w_sum += cp.w;
				J += w * (du - cp.x) * j;

				JJ += w * j * j.t();
			}
		}
		//const float lambda = 5000.f;
		const float lambda = 5000.f*npt/200.f;
		//const float lambda = 5000.f * w_sum/200.0f;
		for (int i = 0; i < 3; ++i)
			JJ(i, i) += lambda * 100.f;

		for (int i = 3; i < 6; ++i)
			JJ(i, i) += lambda;

		int ec = 0;

		Vec6f p;// = -JJ.inv() * J;
		if (solve(JJ, -J, p))
		{
			cv::Vec3f dt(p[0], p[1], p[2]);
			cv::Vec3f rvec(p[3], p[4], p[5]);
			Matx33f dR;
			cv::Rodrigues(rvec, dR);

			pose.t = pose.R * dt + pose.t;
			pose.R = pose.R * dR;


			if (g_uhdl)
				g_uhdl->onUpdate(pose.R, pose.t);

			float diff = p.dot(p);
			//printf("diff=%f\n", sqrt(diff));

			return diff < eps* eps ? 0 : 1;
		}

		return 0;
	}
	bool update(PoseData& pose, const Matx33f& K, const std::vector<CPoint>& cpoints, int maxItrs, float alpha, float eps)
	{
		for (int itr = 0; itr < maxItrs; ++itr)
			if (this->_update(pose, K, cpoints, alpha, eps) <= 0)
				return false;
		return true;
	}
};

struct SecondStageOptimizer
{
public:
	struct ContourPoint
	{
		float   w; //weight
		float   x; //position on the scan-line
	};

	enum { MAX_POINTS_PER_LINE_SECOND = 9 };
	struct ScanLine
	{
		float     y;
		Point2f   xdir;
		Point2f   xstart;
		ContourPoint  vPoints[MAX_POINTS_PER_LINE_SECOND];
		int       nPoints;
		short* cpIndex; //index of the closest contour point for each x position
	public:
		void setCoordinates(const Point2f& start, const Point2f& end, float y)
		{
			this->xstart = start;
			xdir = (Point2f)normalize(Vec2f(end - start));
			this->y = y;
		}
		int getClosestContourPoint(const Point2f& pt, int xsize)
		{
			int x = int((pt - xstart).dot(xdir) + 0.5f);
			if (uint(x) < uint(xsize))
				return cpIndex[x];
			return -1;
		}
	};

	struct DirData
	{
		Vec2f      dir;
		Point2f    ystart;
		Point2f    ydir;
		std::vector<ScanLine>  lines;
		Mat1s         _cpIndexBuf;
	public:
		void setCoordinates(const Point2f& ystart, const Point2f& ypt)
		{
			this->ystart = ystart;
			ydir = (Point2f)normalize(Vec2f(ypt - ystart));
		}
		void resize(int rows, int cols)
		{
			lines.clear();
			lines.resize(rows);
			_cpIndexBuf.create(rows, cols);
			for (int y = 0; y < rows; ++y)
			{
				lines[y].cpIndex = _cpIndexBuf.ptr<short>(y);
			}
		}
		const ScanLine* getScanLine(const Point2f& pt, int& matchedContourPoint)
		{
			int y = int((pt - ystart).dot(ydir) + 0.5f);
			if (uint(y) >= lines.size())
				return nullptr;
			matchedContourPoint = lines[y].getClosestContourPoint(pt, int(_cpIndexBuf.cols));
			return &lines[y];
		}
	};

	std::vector<DirData>  _dirs;
	Rect  _roi;
public:
	static void _gaussianFitting(const float* data, int size, ContourPoint& cp)
	{
		float w = 0.f, wsum = 0.f;
		for (int i = 0; i < size; ++i)
		{
			wsum += data[i] * float(i);
			w += data[i];
		}

		cp.x = wsum / w;
	}
	struct _LineBuilder
	{
		struct LocalMaxima
		{
			int x;
			float val;
		};
		std::vector<LocalMaxima>  _lmBuf;
	public:
		_LineBuilder(int size)
		{
			_lmBuf.resize(size);
		}

		void operator()(ScanLine& line, const float* data, int size, int gaussWindowSizeHalf)
		{
			LocalMaxima* vlm = &_lmBuf[0];
			int nlm = 0;
			for (int i = 1; i < size - 1; ++i)
			{
				if (data[i] > data[i - 1] && data[i] > data[i + 1])
				{
					auto& lm = vlm[nlm++];
					lm.x = i;
					lm.val = data[i];
				}
			}
			int max_nlm = nlm;
			
			if (nlm > MAX_POINTS_PER_LINE_SECOND)
			{
				std::sort(vlm, vlm + nlm, [](const LocalMaxima& a, const LocalMaxima& b) {
					return a.val > b.val;
					});
				nlm = MAX_POINTS_PER_LINE_SECOND;

				std::sort(vlm, vlm + nlm, [](const LocalMaxima& a, const LocalMaxima& b) {
					return a.x < b.x;
					});
			}
			for (int i = 0; i < nlm; ++i)
			{
				auto& lm = vlm[i];
				auto& cp = line.vPoints[i];

				const int start = __max(0, lm.x - gaussWindowSizeHalf), end = __min(size, lm.x + gaussWindowSizeHalf);
				_gaussianFitting(data + start, end - start, cp);

				cp.x += (float)start;
				cp.w = lm.val;
				//cp.w = new_alpha * lm.val;
			}
			line.nPoints = nlm;

			if (nlm <= 1)
				memset(line.cpIndex, nlm == 0 ? 0xFF : 0, sizeof(short) * size);
			else
			{
				int start = 0;
				for (int pi = 0; pi < nlm - 1; ++pi)
				{
					int end = int(int(line.vPoints[pi].x + line.vPoints[pi + 1].x) / 2 + 0.5f) + 1;
					for (int i = start; i < end; ++i)
						line.cpIndex[i] = pi;
					start = end;
				}
				for (int i = start; i < size; ++i)
					line.cpIndex[i] = nlm - 1;
			}
		}
	};
	static void _calcScanLinesForRows(const Mat1f& prob, DirData& dirPositive, DirData& dirNegative, const Matx23f& invA)
	{
		const int gaussWindowSizeHalf = 3;

		Mat1f edgeProb;
		cv::Sobel(prob, edgeProb, CV_32F, 1, 0, 7);


		{
			Point2f O = transA(Point2f(0.f, 0.f), invA.val), P = transA(Point2f(0.f, float(prob.rows - 1)), invA.val);
			dirPositive.setCoordinates(O, P);
			dirNegative.setCoordinates(P, O);
		}
		dirPositive.resize(prob.rows, prob.cols);
		dirNegative.resize(prob.rows, prob.cols);

		std::unique_ptr<float[]> _rdata(new float[prob.cols * 2]);
		float* posData = _rdata.get(), * negData = posData + prob.cols;
		_LineBuilder buildLine(prob.cols);

		const int xend = int(prob.cols - 1);
		for (int y = 0; y < prob.rows; ++y)
		{
			auto& positiveLine = dirPositive.lines[y];
			auto& negativeLine = dirNegative.lines[prob.rows - 1 - y];

			Point2f O = transA(Point2f(0.f, float(y)), invA.val), P = transA(Point2f(float(prob.cols - 1), float(y)), invA.val);
			positiveLine.setCoordinates(O, P, float(y));
			negativeLine.setCoordinates(P, O, float(prob.rows - 1 - y));

			const float* ep = edgeProb.ptr<float>(y);

			for (int x = 0; x < prob.cols; ++x)
			{
				if (ep[x] > 0)
				{
					posData[x] = ep[x]; negData[xend - x] = 0.f;
				}
				else
				{
					posData[x] = 0.f; negData[xend - x] = -ep[x];
				}
			}

			buildLine(positiveLine, posData, prob.cols, gaussWindowSizeHalf);
			buildLine(negativeLine, negData, prob.cols, gaussWindowSizeHalf);
		}
	}

	//Mat  _prob;
	std::vector<int>  _dirIndex;

	void computeScanLines(const Mat1f& prob_, Rect roi)
	{
		//_prob = prob_.clone(); //save for visualization

		const int N = 8;

		Point2f center(float(roi.x + roi.width / 2), float(roi.y + roi.height / 2));
		Rect_<float> roif(roi);
		std::vector<Point2f>  corners = {
			Point2f(roif.x, roif.y), Point2f(roif.x + roif.width,roif.y), Point2f(roif.x + roif.width,roif.y + roif.height),Point2f(roif.x,roif.y + roif.height)
		};

		struct _DDir
		{
			Vec2f   dir;
			Matx23f A;
		};

		_dirs.clear();
		_dirs.resize(N * 2);

		//for (int i = 0; i < N; ++i)
		cv::parallel_for_(cv::Range(0, N), [&](const cv::Range& r) {
			for (int i = r.start; i < r.end; ++i)
			{
				double theta = 180.0 / N * i;
				Matx23f A = getRotationMatrix2D(center, theta, 1.0);
				std::vector<Point2f> Acorners;
				cv::transform(corners, Acorners, A);
				cv::Rect droi = getBoundingBox2D(Acorners);
				A = Matx23f(1, 0, -droi.x,
					0, 1, -droi.y) * A;

				Mat1f dirProb;
				cv::warpAffine(prob_, dirProb, A, droi.size(), INTER_LINEAR, BORDER_CONSTANT, Scalar(0));

				theta = theta / 180.0 * CV_PI;
				auto dir = Vec2f(cos(theta), sin(theta));

				//imshow("dirProb", dirProb);
				//cv::waitKey();

				auto& positiveDir = _dirs[i];
				auto& negativeDir = _dirs[i + N];

				auto invA = invertAffine(A);
				_calcScanLinesForRows(dirProb, positiveDir, negativeDir, invA);

				positiveDir.dir = dir;
				negativeDir.dir = -dir;
			}
			});

		//normalize weight of contour points
		{
			float wMax = 0;
			for (auto& dir : _dirs)
			{
				for (auto& dirLine : dir.lines)
				{
					for (int i = 0; i < dirLine.nPoints; ++i)
						if (dirLine.vPoints[i].w > wMax)
							wMax = dirLine.vPoints[i].w;
				}
			}

			for (auto& dir : _dirs)
			{
				for (auto& dirLine : dir.lines)
				{
					for (int i = 0; i < dirLine.nPoints; ++i)
						dirLine.vPoints[i].w /= wMax;
				}
			}
		}

		//build index of dirs
		if (_dirIndex.empty())
		{
			_dirIndex.resize(361);
			for (int i = 0; i < (int)_dirIndex.size(); ++i)
			{
				float theta = i * CV_PI / 180.f - CV_PI;
				Vec2f dir(cos(theta), sin(theta));

				float cosMax = -1;
				int jm = -1;
				for (int j = 0; j < (int)_dirs.size(); ++j)
				{
					float vcos = _dirs[j].dir.dot(dir);
					if (vcos > cosMax)
					{
						cosMax = vcos;
						jm = j;
					}
				}
				_dirIndex[i] = jm;
			}
		}

		_roi = roi;
	}
	DirData* getDirData(const Vec2f& ptNormal)
	{
		float theta = atan2(ptNormal[1], ptNormal[0]);
		int i = int((theta + CV_PI) * 180 / CV_PI);
		auto* ddx = uint(i) < _dirIndex.size() ? &_dirs[_dirIndex[i]] : nullptr;
		return ddx;
	}

	struct PoseData
		:public Pose
	{
		int itr = 0;
		DFRHandler* hdl = nullptr;
	};

	float calcError(const PoseData& pose, const Matx33f& K, const std::vector<CPoint>& cpoints, float alpha)
	{
		const Matx33f R = pose.R;
		const Point3f t = pose.t;

		const float fx = K(0, 0), fy = K(1, 1), cx = K(0, 2), cy = K(1, 2);

		auto* vcp = &cpoints[0];
		int npt = (int)cpoints.size();
		float err = 0.f;
		float nerr = 0.f;

		for (int i = 0; i < npt; ++i)
		{
			Point3f Q = R * vcp[i].center + t;
			Point3f q = K * Q;
			{
				const int x = int(q.x / q.z + 0.5), y = int(q.y / q.z + 0.5);
				if (uint(x - _roi.x) >= uint(_roi.width) || uint(y - _roi.y) >= uint(_roi.height))
					continue;

				Point3f qn = K * (R * vcp[i].normal + t);
				Vec2f n(qn.x / qn.z - q.x / q.z, qn.y / qn.z - q.y / q.z);
				n = normalize(n);

				Point2f pt(q.x / q.z, q.y / q.z);

				auto* dd = this->getDirData(n);
				if (!dd)
					continue;
				int cpi;
				auto* dirLine = dd->getScanLine(pt, cpi);
				if (!dirLine || cpi < 0)
					continue;

				auto& cp = dirLine->vPoints[cpi];

				Vec2f nx = dirLine->xdir;

				float du = (pt - dirLine->xstart).dot(nx);

				float w = cp.w * cp.w;
				
				err += pow(fabs(du - cp.x), alpha) * w;

				nerr += w;
			}
		}
		return err / nerr;
	}

	int _update(PoseData& pose, const Matx33f& K, const std::vector<CPoint>& cpoints, float alpha, float eps)
	{
		const Matx33f R = pose.R;
		const Point3f t = pose.t;

		if (pose.hdl && pose.hdl->test(R, t) <= 0)
			return 0;

		++g_totalUpdates;

		const float fx = K(0, 0), fy = K(1, 1);

		auto* vcp = &cpoints[0];
		int npt = (int)cpoints.size();

		Matx66f JJ = Matx66f::zeros();
		Vec6f J(0, 0, 0, 0, 0, 0);
		float w_sum = 0.0f;
		for (int i = 0; i < npt; ++i)
		{
			Point3f Q = R * vcp[i].center + t;
			Point3f q = K * Q;
			//if (q.z == 0.f)
			//	continue;
			//else
			{
				const int x = int(q.x / q.z + 0.5), y = int(q.y / q.z + 0.5);
				if (uint(x - _roi.x) >= uint(_roi.width) || uint(y - _roi.y) >= uint(_roi.height))
					continue;

				Point3f qn = K * (R * vcp[i].normal + t);
				Vec2f n(qn.x / qn.z - q.x / q.z, qn.y / qn.z - q.y / q.z);
				n = normalize(n);

				const float X = Q.x, Y = Q.y, Z = Q.z;
				//      |fx/Z   0   -fx*X/Z^2 |   |a  0   b|
				//dq/dQ = |                     | = |        |
				//		|0    fy/Z  -fy*Y/Z^2 |   |0  c   d|
				//
				const float a = fx / Z, b = -fx * X / (Z * Z), c = fy / Z, d = -fy * Y / (Z * Z);

				Point2f pt(q.x / q.z, q.y / q.z);

				auto* dd = this->getDirData(n);
				if (!dd)
					continue;
				int cpi;
				auto* dirLine = dd->getScanLine(pt, cpi);
				if (!dirLine || cpi < 0)
					continue;

				auto& cp = dirLine->vPoints[cpi];

				Vec2f nx = dirLine->xdir;
				float du = (pt - dirLine->xstart).dot(nx);

				Vec3f n_dq_dQ(nx[0] * a, nx[1] * c, nx[0] * b + nx[1] * d);

				auto dt = n_dq_dQ.t() * R;
				auto dR = vcp[i].center.cross(Vec3f(dt.val[0], dt.val[1], dt.val[2]));

				Vec6f j(dt.val[0], dt.val[1], dt.val[2], dR.x, dR.y, dR.z);

				float w = pow(1.f / (fabs(du - cp.x) + 1.f), 2.f - alpha) * cp.w * cp.w;
				
				w_sum += cp.w;
				J += w * (du - cp.x) * j;

				JJ += w * j * j.t();
			}
		}

		//const float lambda = 5000.f;
		const float lambda = 5000.f * npt / 200.f;
		//const float lambda = 5000.f * w_sum/200.0f;
		for (int i = 0; i < 3; ++i)
			JJ(i, i) += lambda * 100.f;

		for (int i = 3; i < 6; ++i)
			JJ(i, i) += lambda;

		int ec = 0;

		Vec6f p;// = -JJ.inv() * J;
		if (solve(JJ, -J, p))
		{
			cv::Vec3f dt(p[0], p[1], p[2]);
			cv::Vec3f rvec(p[3], p[4], p[5]);
			Matx33f dR;
			cv::Rodrigues(rvec, dR);

			pose.t = pose.R * dt + pose.t;
			pose.R = pose.R * dR;


			if (g_uhdl)
				g_uhdl->onUpdate(pose.R, pose.t);

			float diff = p.dot(p);
			//printf("diff=%f\n", sqrt(diff));

			return diff < eps* eps ? 0 : 1;
		}

		return 0;
	}
	bool update(PoseData& pose, const Matx33f& K, const std::vector<CPoint>& cpoints, int maxItrs, float alpha, float eps)
	{
		for (int itr = 0; itr < maxItrs; ++itr)
			if (this->_update(pose, K, cpoints, alpha, eps) <= 0)
				return false;
		return true;
	}
};

struct Templates
{
	Point3f               modelCenter;
	std::vector<DView>   views;
	ViewIndex             viewIndex;

	DEFINE_BFS_IO_2(Templates, modelCenter, views)

public:
	void build(CVRModel& model);

	void save(const std::string& file)
	{
		ff::OBFStream os(file);
		os << (*this);
	}
	void load(const std::string& file)
	{
		ff::IBFStream is(file);
		is >> (*this);
		viewIndex.build(this->views);
	}
	void showInfo()
	{
		int minSamplePoints = INT_MAX;
		for (auto& v : views)
			if (v.contourPoints3d.size() < minSamplePoints)
				minSamplePoints = v.contourPoints3d.size();
		printf("minSamplePoints=%d\n", minSamplePoints);
	}
public:
	Vec3f _getViewDir(const Matx33f& R, const Vec3f& t)
	{
		return normalize(-R.inv() * t - Vec3f(this->modelCenter));
	}
	int  _getNearestView(const Vec3f& viewDir)
	{
		CV_Assert(fabs(viewDir.dot(viewDir) - 1.f) < 1e-3f);

		return viewIndex.getViewInDir(viewDir);
	}
	int  _getNearestView(const Matx33f& R, const Vec3f& t)
	{
		return _getNearestView(this->_getViewDir(R, t));
	}

	float pro(const Matx33f& K, Pose& pose, const Mat1f& curProb, float thetaT, float errT)
	{
		return this->pro1(K, pose, curProb, thetaT, errT);
	}

	float pro1(const Matx33f& K, Pose& pose, const Mat1f& curProb, float thetaT, float errT);
};


class RegionTrajectory
{
	Mat1b  _pathMask;
	float  _delta;

	Point2f _uv2Pt(const Point2f& uv)
	{
		return Point2f(uv.x / _delta + float(_pathMask.cols) / 2.f, uv.y / _delta + float(_pathMask.rows) / 2.f);
	}
public:
	RegionTrajectory(Size regionSize, float delta)
	{
		_pathMask = Mat1b::zeros(regionSize);
		_delta = delta;
	}
	bool  addStep(Point2f start, Point2f end)
	{
		start = _uv2Pt(start);
		end = _uv2Pt(end);

		auto dv = end - start;
		float len = sqrt(dv.dot(dv)) + 1e-6f;
		float dx = dv.x / len, dy = dv.y / len;
		const int  N = int(len) + 1;
		Point2f p = start;
		for (int i = 0; i < N; ++i)
		{
			int x = int(p.x + 0.5), y = int(p.y + 0.5);
			if (uint(x) < uint(_pathMask.cols) && uint(y) < uint(_pathMask.rows))
			{
				if (_pathMask(y, x) != 0 && len > 1.f)
					return true;
				_pathMask(y, x) = 1;
			}
			else
				return false;

			p.x += dx; p.y += dy;
		}

		return false;
	}

};

inline float Templates::pro1(const Matx33f& K, Pose& pose, const Mat1f& curProb, float thetaT, float errT)
{
	Rect curROI;
	int curView = this->_getNearestView(pose.R, pose.t);
	{
		Projector prj(K, pose.R, pose.t);
		std::vector<Point2f>  c2d = prj(views[curView].contourPoints3d, [](const CPoint& p) {return p.center; });
		Rect_<float> rectf = getBoundingBox2D(c2d);
		curROI = Rect(rectf);
	}

	FirstStageOptimizer dfr;
	
	Rect roi = curROI;
	const int dW = 100;
	rectAppend(roi, dW, dW, dW, dW);
	roi = rectOverlapped(roi, Rect(0, 0, curProb.cols, curProb.rows));
	dfr.computeScanLines(curProb, roi);

	//printf("init time=%dms \n", int(clock() - beg));

	FirstStageOptimizer::PoseData dpose;
	static_cast<Pose&>(dpose) = pose;
	//change alpha here
	const float alpha = 0.125f, alphaNonLocal=0.75f, eps=1e-4f;
	const int outerItrs = 10, innerItrs = 3;

	for (int itr = 0; itr < outerItrs; ++itr)
	{
		curView = this->_getNearestView(dpose.R, dpose.t);
		if (!dfr.update(dpose, K, this->views[curView].contourPoints3d, innerItrs, alpha, eps))
			break;
	}

	float errMin = dfr.calcError(dpose, K, this->views[curView].contourPoints3d,alpha);
	if (errMin > errT)
	//if(false)
	{
		const auto R0 = dpose.R;

		const int N = int(thetaT / (CV_PI / 12) + 0.5f) | 1;

		const float dc = thetaT / N;

		const int subDiv = 3;
		const int subRegionSize = (N * 2 * 2 + 1) * subDiv;
		RegionTrajectory traj(Size(subRegionSize, subRegionSize), dc / subDiv);

		Mat1b label = Mat1b::zeros(2 * N + 1, 2 * N + 1);
		struct DSeed
		{
			Point coord;
			//bool  isLocalMinima;
		};
		std::deque<DSeed>  seeds;
		seeds.push_back({ Point(N,N)/*,true*/ });
		label(N, N) = 1;


		auto checkAdd = [&seeds, &label](const DSeed& curSeed, int dx, int dy) {
			int x = curSeed.coord.x + dx, y = curSeed.coord.y + dy;
			if (uint(x) < uint(label.cols) && uint(y) < uint(label.rows))
			{
				if (label(y, x) == 0)
				{
					label(y, x) = 1;
					seeds.push_back({ Point(x, y)/*, false*/ });
				}
			}
		};

		while (!seeds.empty())
		{
			auto curSeed = seeds.front();
			seeds.pop_front();

			checkAdd(curSeed, 0, -1);
			checkAdd(curSeed, -1, 0);
			checkAdd(curSeed, 1, 0);
			checkAdd(curSeed, 0, 1);

			auto dR = theta2OutofplaneRotation(float(curSeed.coord.x - N) * dc, float(curSeed.coord.y - N) * dc);

			auto dposex = dpose;
			dposex.R = dR * R0;

			Point2f start = dir2Theta(viewDirFromR(dposex.R * R0.t()));
			for (int itr = 0; itr < outerItrs*innerItrs; ++itr)
			{
				if (itr % innerItrs == 0)
					curView = this->_getNearestView(dposex.R, dposex.t);

				if (!dfr.update(dposex, K, this->views[curView].contourPoints3d, 1, alphaNonLocal, eps))
					break;

				Point2f end = dir2Theta(viewDirFromR(dposex.R * R0.t()));
				if (traj.addStep(start, end))
					break;
				start = end;
			}
			{
				curView = this->_getNearestView(dposex.R, dposex.t);
				float err = dfr.calcError(dposex, K, this->views[curView].contourPoints3d, alpha);
				if (err < errMin)
				{
					errMin = err;
					dpose = dposex;
				}
				if (errMin < errT)
					break;
			}
		}
	}

	for (int itr = 0; itr < outerItrs; ++itr)
	{
		curView = this->_getNearestView(dpose.R, dpose.t);
		if (!dfr.update(dpose, K, this->views[curView].contourPoints3d, innerItrs, alpha, eps))
			break;
	}
	
	
	curView = this->_getNearestView(dpose.R, dpose.t);
	{
		Projector prj(K, dpose.R, dpose.t);
		std::vector<Point2f>  c2d = prj(views[curView].contourPoints3d, [](const CPoint& p) {return p.center; });
		Rect_<float> rectf = getBoundingBox2D(c2d);
		curROI = Rect(rectf);
	}
	
	SecondStageOptimizer dfrSnd;
	roi = curROI;
	rectAppend(roi, dW, dW, dW, dW);
	roi = rectOverlapped(roi, Rect(0, 0, curProb.cols, curProb.rows));
	dfrSnd.computeScanLines(curProb, roi);
	SecondStageOptimizer::PoseData dposeSnd;
	static_cast<Pose&>(dposeSnd) = dpose;
	for (int itr = 0; itr < outerItrs; ++itr)
	{
		curView = this->_getNearestView(dposeSnd.R, dposeSnd.t);
		if (!dfrSnd.update(dposeSnd, K, this->views[curView].contourPoints3d, innerItrs, alpha, eps))
			break;
	}
	pose = dposeSnd;
	
	return errMin;
}


inline void Templates::build(CVRModel& model)
{
	std::vector<Vec3f>  viewDirs;
	cvrm::sampleSphere(viewDirs, 3000);
	
	auto center = model.getCenter();
	auto sizeBB = model.getSizeBB();
	float maxBBSize = __max(sizeBB[0], __max(sizeBB[1], sizeBB[2]));
	float eyeDist = 0.8f;
	float fscale = 2.5f;
	Size  viewSize(2000, 2000);

	this->modelCenter = center;

	std::vector<DView> dviews;
	dviews.reserve(viewDirs.size());

	CVRender render(model);

	auto vertices = model.getVertices();

	int vi = 1;
	int nSamples = 200;
	for (auto& viewDir : viewDirs)
	{
		printf("build templates %d/%d    \r", vi++, (int)viewDirs.size());
		{
			auto eyePos = center + viewDir * eyeDist;

			CVRMats mats;
			mats.mModel = cvrm::lookat(eyePos[0], eyePos[1], eyePos[2], center[0], center[1], center[2], 0.1f, 1.1f, 0.1f);
			mats.mProjection = cvrm::perspective(viewSize.height * fscale, viewSize, __max(0.01, eyeDist - maxBBSize), eyeDist + maxBBSize);

			auto rr = render.exec(mats, viewSize);
			Mat1b fgMask = getRenderMask(rr.depth);

			{
				auto roi = get_mask_roi(DWHS(fgMask), 127);
				int bwx = __min(roi.x, fgMask.cols - roi.x - roi.width);
				int bwy = __min(roi.y, fgMask.rows - roi.y - roi.height);
				if (__min(bwx, bwy) < 5/* || __max(roi.width,roi.height)<fgMask.cols/4*/)
				{
					imshow("mask", fgMask);
					cv::waitKey();
				}
			}

			dviews.push_back(DView());
			DView& dv = dviews.back();

			dv.viewDir = viewDir;

			Vec3f t;
			cvrm::decomposeRT(mats.mModel, dv.R, t);

			EdgeSampler::sample(dv.contourPoints3d, rr, nSamples);
		}
	}
	this->views.swap(dviews);
	this->viewIndex.build(this->views);
}


struct Frame
{
	Mat3b    img;
	Mat1b    objMask;
	Mat1f    colorProb;
	Pose	 pose;
	float    err;
};

struct Object
{
	CVRModel  model;
	CVRender  render;
	std::string modelFile;
	Templates   templ;
public:
	void loadModel(const std::string& modelFile, float modelScale, bool forceRebuild = false)
	{
		model.load(modelFile, 0);
		model.transform(cvrm::scale(modelScale, modelScale, modelScale));

		render = CVRender(model);
		this->modelFile = modelFile;

		auto tfile = ff::ReplacePathElem(modelFile, "tm034", ff::RPE_FILE_EXTENTION); //034(LDT3D)
		if (!forceRebuild && ff::pathExist(tfile))
			templ.load(tfile);
		else
		{
			templ.build(this->model);
			templ.save(tfile);
		}
		//templ.showInfo();
	}
	cv::Vec3f getObjectBBox()
	{
		return model.getSizeBB();
	}
	vector<cv::Vec3f> getObjectPoints()
	{
		return model.getVertices();
	}
};

inline Mat1b renderObjMask(Object& obj, const Pose& pose, const Matx44f& mProj, Size dsize)
{
	CVRMats mats;
	mats.mModel = cvrm::fromR33T(pose.R, pose.t);
	mats.mProjection = mProj;

	CVRResult rr = obj.render.exec(mats, dsize, CVRM_DEPTH | CVRM_IMAGE);
	//imshow("rr", rr.img);

	return getRenderMask(rr.depth);
}

class ColorHistogram
{
	static const int CLR_RSB = 3;
	static const int TAB_WIDTH = (1 << (8 - CLR_RSB)); //+2, border
	static const int TAB_WIDTH_2 = TAB_WIDTH * TAB_WIDTH;
	static const int TAB_SIZE = TAB_WIDTH * TAB_WIDTH_2;

	static int _color_index(const uchar* pix)
	{
		return (int(pix[0]) >> CLR_RSB) * TAB_WIDTH_2 + (int(pix[1]) >> CLR_RSB) * TAB_WIDTH + (int(pix[2]) >> CLR_RSB);
	}

	struct TabItem
	{
		float nbf[2];
	};

	int _consideredLength = 20, _unconsiderLength = 1;
	std::vector<TabItem>  _tab, _dtab;

	static void _do_update(TabItem* tab, const TabItem* dtab, float learningRate, float dtabSum[2])
	{
		float tscale = 1.f - learningRate;
		float dscale[] = { learningRate / dtabSum[0], learningRate / dtabSum[1] };

		for (int i = 0; i < TAB_SIZE; ++i)
		{
			for (int j = 0; j < 2; ++j)
			{
				tab[i].nbf[j] = tab[i].nbf[j] * tscale + dtab[i].nbf[j] * dscale[j];
			}
		}
	}

public:
	ColorHistogram()
	{
		_tab.resize(TAB_SIZE);
		_dtab.resize(TAB_SIZE);
	}
	void update(Object& obj, const Mat3b& img, const Pose& pose, const Matx33f& K, float learningRate)
	{
		//learningRate = 0.1f;

		auto& templ = obj.templ;
		auto modelCenter = obj.model.getCenter();

		Projector prj(K, pose.R, pose.t);

		TabItem* dtab = &_dtab[0];
		memset(dtab, 0, sizeof(dtab[0]) * _dtab.size());
		float dtabSum[2] = { 0.f,0.f };
		auto addPixel = [&img, dtab, &dtabSum](const Point2f& p, const int fg_bg_i) {
			int x = int(p.x + 0.5f), y = int(p.y + 0.5f);
			if (uint(x) < uint(img.cols) && uint(y) < uint(img.rows))
			{
				const uchar* p = img.ptr(y, x);
				dtab[_color_index(p)].nbf[fg_bg_i] += 1.f;
				dtabSum[fg_bg_i] += 1.f;
				return true;
			}
			return false;
		};

		int curView = templ._getNearestView(pose.R, pose.t);
		if (uint(curView) < templ.views.size())
		{
			Point2f objCenter = prj(modelCenter);

			auto& view = templ.views[curView];
			for (auto& cp : view.contourPoints3d)
			{
				Point2f c = prj(cp.center);
				Point2f n = objCenter - c;
				const float fgLength = sqrt(n.dot(n));
				n = n * (1.f / fgLength);

				Point2f pt = c + float(_unconsiderLength) * n;
				int end = __min(_consideredLength, int(fgLength));
				for (int i = _unconsiderLength; i < end; ++i, pt += n)
				{
					if (!addPixel(pt, 1))
						break;
				}
				end = _consideredLength * 4;
				pt = c - float(_unconsiderLength) * n;
				for (int i = _unconsiderLength; i < end; ++i, pt -= n)
				{
					if (!addPixel(pt, 0))
						break;
				}
			}
			_do_update(&_tab[0], dtab, learningRate, dtabSum);
		}
	}
	Mat1f getProb(const Mat3b& img)
	{
		Mat1f prob(img.size());
		const TabItem* tab = &_tab[0];
		for_each_2(DWHNC(img), DN1(prob), [this, tab](const uchar* c, float& p) {
			int ti = _color_index(c);
			auto& nbf = tab[ti].nbf;
			p = (nbf[1] + 1e-6f) / (nbf[0] + nbf[1] + 2e-6f);
			});
		
		return prob;
	}
};


inline float getRDiff(const cv::Matx33f& R1, const cv::Matx33f& R2)
{
	cv::Matx33f tmp = R1.t() * R2;
	float cmin = __min(__min(tmp(0, 0), tmp(1, 1)), tmp(2, 2));
	return acos(cmin);
}

class Tracker
	:public ITracker
{
	float       _modelScale = 0.001f;
	float       _histogramLearningRate = 0.2f;
	cv::Matx33f _K;
	cv::Matx44f _mProj;
	Frame       _prev;
	Frame       _cur;
	Object  _obj;
	ColorHistogram _colorHistogram;
	bool    _isLocalTracking = false;

	struct FrameInfo
	{
		float   theta;
		float   err;
	};

	std::deque<FrameInfo>  _frameInfo;

	Pose _scalePose(const Pose& pose) const
	{
		return { pose.R, pose.t * _modelScale };
	}
	Pose _descalePose(const Pose& pose) const
	{
		return { pose.R, pose.t * (1.f / _modelScale) };
	}
public:
	virtual void loadModel(const std::string& modelFile, const std::string&argstr)
	{
		_obj.loadModel(modelFile, _modelScale);

		ff::CommandArgSet args(argstr);
		_isLocalTracking = args.getd<bool>("local", false);
	}
	virtual void setUpdateHandler(UpdateHandler* hdl)
	{
		g_uhdl = hdl;
	}
	virtual void reset(const Mat& img, const Pose& pose, const Matx33f& K)
	{
		_cur.img = img.clone();
		_cur.pose = _scalePose(pose);
		_cur.objMask = Mat1b();
		_mProj = cvrm::fromK(K, img.size(), 0.1, 3);
		_K = K;

		_colorHistogram.update(_obj, _cur.img, _cur.pose, _K, 1.f);
	}
	virtual void startUpdate(const Mat& img, Pose gtPose = Pose())
	{
		if (!_prev.img.empty())
		{
			float	theta = getRDiff(_prev.pose.R, _cur.pose.R);
			_frameInfo.push_back({ theta, _cur.err });
			while (_frameInfo.size() > 30)
				_frameInfo.pop_front();
		}

		_prev = _cur;
		_cur.img = img;
		_cur.colorProb = _colorHistogram.getProb(_cur.img);
	}

	template<typename _GetValT>
	static float _getMedianOfLastN(const std::deque<FrameInfo>& frameInfo, int nMax, _GetValT getVal)
	{
		int n = __min((int)frameInfo.size(), nMax);
		std::vector<float> tmp(n);
		int i = 0;
		for (auto itr = frameInfo.rbegin(); i < n; ++itr)
		{
			tmp[i++] = getVal(*itr);
		}
		std::sort(tmp.begin(), tmp.end());
		return tmp[n / 2];
	}

	virtual float update(Pose& pose)
	{
		_cur.pose = _scalePose(pose);

		float thetaT = CV_PI / 8, errT = 1.f; //default values used for only the first 2 frames

		if (!_frameInfo.empty())
		{//estimate from previous frames
			thetaT = _getMedianOfLastN(_frameInfo, 5, [](const FrameInfo& v) {return v.theta; });
			errT = _getMedianOfLastN(_frameInfo, 15, [](const FrameInfo& v) {return v.err; });
		}
		if (isnan(thetaT)) thetaT = CV_PI / 8;
		if (isnan(errT)) errT = 1.f;
		_cur.err = _obj.templ.pro(_K, _cur.pose, _cur.colorProb, thetaT, _isLocalTracking? FLT_MAX : errT);
		pose = _descalePose(_cur.pose);
		return _cur.err;
	}

	virtual void endUpdate(const Pose& pose)
	{
		_cur.pose = _scalePose(pose);
		_colorHistogram.update(_obj, _cur.img, _cur.pose, _K, _histogramLearningRate);
	}
};


_TRACKER_END()


