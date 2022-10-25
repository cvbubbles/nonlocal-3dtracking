
#include"cvrender.h"

//#include"GL/glut.h"
#include"glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include"opencv2/calib3d.hpp"
using namespace cv;
#include<set>

template<typename _DestT, typename _T>
inline _DestT& cvt(const _T &m)
{
	return *(_DestT*)&m;
}
inline Matx44f& cvtm(const glm::mat4 &m)
{
	return cvt<Matx44f>(m);
}
inline glm::mat4& cvtm(const Matx44f &m)
{
	return cvt<glm::mat4>(m);
}


Matx44f cvrm::diag(float s)
{
	auto m = glm::mat4(s);
	return cvtm(m);
}

Matx44f cvrm::translate(float tx, float ty, float tz)
{
	glm::mat4 m(1.0f);
	m = glm::translate(m,glm::vec3(tx, ty, tz));

	return cvtm(m);
}

Matx44f cvrm::scale(float sx, float sy, float sz)
{
	glm::mat4 m(1.0f);
	m = glm::scale(m, glm::vec3(sx, sy, sz));
	return cvtm(m);
}

Matx44f cvrm::rotate(float angle, const cv::Vec3f &axis)
{
	glm::mat4 m(1.0f);
	m=glm::rotate(m, angle, cvt<glm::vec3>(axis));
	return cvtm(m);
}
Matx44f cvrm::rotate(const float r0[3], const float r1[3], const float r2[3])
{
	Matx44f m = cvrm::diag(1.0f);

	float *v = m.val;
	v[0] = r0[0]; v[1] = r1[0]; v[2] = r2[0];
	v[4] = r0[1]; v[5] = r1[1]; v[6] = r2[1];
	v[8] = r0[2]; v[9] = r1[2]; v[10] = r2[2];

	return m;
}
Matx44f cvrm::ortho(float left, float right, float bottom, float top, float nearP, float farP)
{
	auto m = glm::ortho(left, right, bottom, top, nearP, farP);
	return cvtm(m);
}

Matx44f cvrm::perspective(float f, Size windowSize, float nearP, float farP)
{
#if 0
	auto m=glm::perspectiveFov<float>(2 * atan2(windowSize.height / 2.0f, f), windowSize.width, windowSize.height, nearP, farP);
	return cvtm(m);
#else
	return cvrm::perspective(f, f, windowSize.width / 2, windowSize.height / 2, windowSize, nearP, farP);
#endif
}

Matx44f cvrm::perspective(float fx, float fy, float cx, float cy, Size windowSize, float nearP, float farP)
{
	Matx44f m;
	float *mProjection = m.val;

	const int width = windowSize.width, height = windowSize.height;

	/*T const rad = fov;
	T const h = glm::cos(static_cast<T>(0.5) * rad) / glm::sin(static_cast<T>(0.5) * rad);
	T const w = h * height / width; ///todo max(width , Height) / min(width , Height)?

	tmat4x4<T, defaultp> Result(static_cast<T>(0));
	Result[0][0] = w;
	Result[1][1] = h;
	Result[2][2] = -(zFar + zNear) / (zFar - zNear);
	Result[2][3] = -static_cast<T>(1);
	Result[3][2] = -(static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
	return Result;*/

#if 1
	// in opengl context, the matrix is stored in column-major order 
	mProjection[0] = 2.0f * fx / width;
	mProjection[1] = 0.0f;
	mProjection[2] = 0.0f;
	mProjection[3] = 0.0f;

	mProjection[4] = 0.0f;
	mProjection[5] = 2.0f * fy / height;
	mProjection[6] = 0.0f;
	mProjection[7] = 0.0f;


	mProjection[8] = 1.0f - 2.0f * cx / width;
	mProjection[9] = -1.0f + 2.0f * cy / height; 
	mProjection[10] = -(farP + nearP) / (farP - nearP);
	mProjection[11] = -1.0f;

	mProjection[12] = 0.0f;
	mProjection[13] = 0.0f;
	mProjection[14] = -2.0f * farP * nearP / (farP - nearP);
	mProjection[15] = 0.0f;
#else
	mProjection[0] = 2.0f * fx / width;
	mProjection[1] = 0.0f;
	mProjection[2] = 0.0f;
	mProjection[3] = 0.0f;

	mProjection[4] = 0.0f;
	mProjection[5] = -2.0f * fy / height;
	mProjection[6] = 0.0f;
	mProjection[7] = 0.0f;


	mProjection[8] = 1.0f - 2.0f * cx / width;
	mProjection[9] = -1.0f + 2.0f * cy / height;
	mProjection[10] = (farP + nearP) / (farP - nearP);
	mProjection[11] = -1.0f;

	mProjection[12] = 0.0f;
	mProjection[13] = 0.0f;
	mProjection[14] = 2.0f * farP * nearP / (farP - nearP);
	mProjection[15] = 0.0f;
#endif

	return m;
}

Matx33f cvrm::defaultK(Size imageSize, float fscale)
{
	Matx33f K = Matx33f::eye();

	float f=imageSize.height*fscale;

	K(0, 0) = f;
	K(1, 1) = f;
	K(0, 2) = float(imageSize.width)/2.0f;
	K(1, 2) = float(imageSize.height)/2.0f;

	return K;
}

Matx33f cvrm::scaleK(const Matx33f& K, float scalex, float scaley)
{
	Matx33f dK = K;
	for (int i = 0; i < 3; ++i)
		dK(0, i) *= scalex;
	for (int i = 0; i < 3; ++i)
		dK(1, i) *= scaley;
	return dK;
}

Matx44f cvrm::fromK(const Matx33f &K, Size windowSize, float nearP, float farP)
{
	cv::Mat1f Kf(K);
	return perspective(Kf(0, 0), Kf(1, 1), Kf(0, 2), Kf(1, 2), windowSize, nearP, farP);
}

Matx44f cvrm::lookat(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz)
{
	auto m = glm::lookAt(glm::vec3(eyex, eyey, eyez), glm::vec3(centerx, centery, centerz), glm::vec3(upx, upy, upz));
	return cvtm(m);
}

static void getGLModelView(const cv::Matx33f &R, const cv::Vec3f &t, float* mModelView)
{
	//绕X轴旋转180度，从OpenCV坐标系变换为OpenGL坐标系
	//同时转置，opengl默认的矩阵为列主序
#if 1
	mModelView[0] = R(0, 0);
	mModelView[1] = -R(1, 0);
	mModelView[2] = -R(2, 0);
	mModelView[3] = 0.0f;

	mModelView[4] = R(0, 1);
	mModelView[5] = -R(1, 1);
	mModelView[6] = -R(2, 1);
	mModelView[7] = 0.0f;

	mModelView[8] = R(0, 2);
	mModelView[9] = -R(1, 2);
	mModelView[10] = -R(2, 2);
	mModelView[11] = 0.0f;

	mModelView[12] = t(0);
	mModelView[13] = -t(1);
	mModelView[14] = -t(2);
	mModelView[15] = 1.0f;
#else
	mModelView[0] = R(0, 0);
	mModelView[1] = R(1, 0);
	mModelView[2] = R(2, 0);
	mModelView[3] = 0.0f;

	mModelView[4] = R(0, 1);
	mModelView[5] = R(1, 1);
	mModelView[6] = R(2, 1);
	mModelView[7] = 0.0f;

	mModelView[8] = R(0, 2);
	mModelView[9] = R(1, 2);
	mModelView[10] = R(2, 2);
	mModelView[11] = 0.0f;

	mModelView[12] = t(0);
	mModelView[13] = t(1);
	mModelView[14] = t(2);
	mModelView[15] = 1.0f;
#endif
}
Matx44f cvrm::fromRT(const Vec3f &rvec, const Vec3f &tvec)
{
	if (isinf(tvec[0]) || isnan(tvec[0]) || isinf(rvec[0]) || isnan(rvec[0]))
		return I();

	cv::Matx33f R;
	cv::Rodrigues(rvec, R);

	return fromR33T(R,tvec);
}
Matx44f cvrm::fromR33T(const cv::Matx33f &R, const cv::Vec3f &tvec)
{
	cv::Matx44f m;
	getGLModelView(R, tvec, m.val);
	return m;
}

void cvrm::decomposeRT(const Matx44f &m, Vec3f &rvec, Vec3f &tvec)
{
	cv::Matx33f R;
	decomposeRT(m, R, tvec);
	cv::Rodrigues(R, rvec);
}

void cvrm::decomposeRT(const Matx44f &m, cv::Matx33f &R, cv::Vec3f &tvec)
{
	const float *v = m.val;
	CV_Assert(fabs(v[3]) < 1e-3f && fabs(v[7]) < 1e-3&&fabs(v[11]) < 1e-3&&fabs(v[15] - 1.0f) < 1e-3f);
#if 1
	tvec[0] = v[12];
	tvec[1] = -v[13];
	tvec[2] = -v[14];

	R=cv::Matx33f(v[0], v[4], v[8], -v[1], -v[5], -v[9], -v[2], -v[6], -v[10]);
#else
	tvec[0] = v[12];
	tvec[1] = v[13];
	tvec[2] = v[14];

	R = cv::Matx33f(v[0], v[4], v[8], v[1], v[5], v[9], v[2], v[6], v[10]);
#endif
}

#if 0
static cv::Mat flipExtrinsicYZ(const cv::Mat &m_extrinsic)
{
	// rotate along x with 180
	// the rotation matrix Rx is  
	// [1 0  0]
	// [0 -1 0]
	// [0 0 -1]
	// out = Rx x in
	cv::Mat flipedExtrinsic = m_extrinsic.clone();

	flipedExtrinsic.ptr<float>(1)[0] = -m_extrinsic.ptr<float>(1)[0];
	flipedExtrinsic.ptr<float>(1)[1] = -m_extrinsic.ptr<float>(1)[1];
	flipedExtrinsic.ptr<float>(1)[2] = -m_extrinsic.ptr<float>(1)[2];
	flipedExtrinsic.ptr<float>(1)[3] = -m_extrinsic.ptr<float>(1)[3];

	flipedExtrinsic.ptr<float>(2)[0] = -m_extrinsic.ptr<float>(2)[0];
	flipedExtrinsic.ptr<float>(2)[1] = -m_extrinsic.ptr<float>(2)[1];
	flipedExtrinsic.ptr<float>(2)[2] = -m_extrinsic.ptr<float>(2)[2];
	flipedExtrinsic.ptr<float>(2)[3] = -m_extrinsic.ptr<float>(2)[3];

	return flipedExtrinsic;
}

Matx44f cvrm::rt(float rx, float ry, float rz, float tx, float ty, float tz)
{
	cv::Mat rvmat = cv::Mat::zeros(1, 3, CV_32FC1);
	rvmat.ptr<float>(0)[0] = rx; rvmat.ptr<float>(0)[1] = ry; rvmat.ptr<float>(0)[2] = rz;
	cv::Mat rmat = cv::Mat::zeros(3, 3, CV_32FC1);
	cv::Rodrigues(rvmat, rmat);

	cv::Mat1f m_extrinsic(3, 4);

	m_extrinsic.ptr<float>(0)[0] = rmat.ptr<float>(0)[0];
	m_extrinsic.ptr<float>(0)[1] = rmat.ptr<float>(0)[1];
	m_extrinsic.ptr<float>(0)[2] = rmat.ptr<float>(0)[2];
	m_extrinsic.ptr<float>(0)[3] = tx;

	m_extrinsic.ptr<float>(1)[0] = rmat.ptr<float>(1)[0];
	m_extrinsic.ptr<float>(1)[1] = rmat.ptr<float>(1)[1];
	m_extrinsic.ptr<float>(1)[2] = rmat.ptr<float>(1)[2];
	m_extrinsic.ptr<float>(1)[3] = ty;

	m_extrinsic.ptr<float>(2)[0] = rmat.ptr<float>(2)[0];
	m_extrinsic.ptr<float>(2)[1] = rmat.ptr<float>(2)[1];
	m_extrinsic.ptr<float>(2)[2] = rmat.ptr<float>(2)[2];
	m_extrinsic.ptr<float>(2)[3] = tz;

	// from opencv context to opengl context
	// we need rotate the object frame along X with 180 degrees
	cv::Mat flipedExtrinsic = flipExtrinsicYZ(m_extrinsic);

	Matx44f m;
	float *mModelview = m.val;

	// in opengl context, the matrix is stored in column-major order 
	mModelview[0] = flipedExtrinsic.ptr<float>(0)[0];
	mModelview[1] = flipedExtrinsic.ptr<float>(1)[0];
	mModelview[2] = flipedExtrinsic.ptr<float>(2)[0];
	mModelview[3] = 0.0f;

	mModelview[4] = flipedExtrinsic.ptr<float>(0)[1];
	mModelview[5] = flipedExtrinsic.ptr<float>(1)[1];
	mModelview[6] = flipedExtrinsic.ptr<float>(2)[1];
	mModelview[7] = 0.0f;


	mModelview[8] = flipedExtrinsic.ptr<float>(0)[2];
	mModelview[9] = flipedExtrinsic.ptr<float>(1)[2];
	mModelview[10] = flipedExtrinsic.ptr<float>(2)[2];
	mModelview[11] = 0.0f;


	mModelview[12] = flipedExtrinsic.ptr<float>(0)[3];
	mModelview[13] = flipedExtrinsic.ptr<float>(1)[3];
	mModelview[14] = flipedExtrinsic.ptr<float>(2)[3];
	mModelview[15] = 1.0f;

	return m;
}
#endif

cv::Point3f cvrm::unproject(const cv::Point3f &winPt, const Matx44f &mModelview, const Matx44f &mProjection, const int viewport[4])
{
	glm::vec3 p=glm::unProject(cvt<glm::vec3>(winPt), cvtm(mModelview), cvtm(mProjection), glm::vec4(viewport[0], viewport[1], viewport[2], viewport[3]));
	return cvt<cv::Point3f>(p);
}

cv::Point3f cvrm::project(const cv::Point3f &objPt, const Matx44f &mModelview, const Matx44f &mProjection, const int viewport[4])
{
	glm::vec3 p = glm::project(cvt<glm::vec3>(objPt), cvtm(mModelview), cvtm(mProjection), glm::vec4(viewport[0], viewport[1], viewport[2], viewport[3]));
	return cvt<cv::Point3f>(p);
}

static Vec3f _getVertical(const Vec3f &v)
{
	Vec3f u;
	if (fabs(v[0]) <1e-3f && fabs(v[1]) <1e-3f)
		u = v.cross(Vec3f(1, 0, 0));
	else
		u = v.cross(Vec3f(0, 0, 1));

	return u;
}

Matx44f  cvrm::rotate(const cv::Vec3f &u, const cv::Vec3f &v)
{
	CV_Assert(fabs(u.dot(u) - 1.0) < 1e-3 && fabs(v.dot(v) - 1.0) < 1e-3);

	double angle = acos(u.ddot(v));
	Vec3f axis = u.cross(v);
	if (axis.dot(axis) < 1e-6f)
	{
		axis = _getVertical(u);
	}
	return cvrm::rotate(angle, axis);
}

void  cvrm::sampleSphere(std::vector<cv::Vec3f> &vecs, int N)
{
	vecs.clear();
	if (N > 1)
	{
		vecs.reserve(N);
		const double phi = (sqrt(5.0) - 1) / 2;
		for (int n = 0; n < N; ++n)
		{
			double z = double(2 * n) / (N-1) - 1;
			double r = sqrt(1 - z*z);
			double x = r*cos(2 * CV_PI*n*phi);
			double y = r*sin(2 * CV_PI*n*phi);
			vecs.push_back(normalize(Vec3f(x, y, z)));
		}
	}
}

cv::Vec3f cvrm::rot2Euler(const cv::Matx33f &R)
{
	cv::Vec3f eulerxyz(0, 0, 0);

	if (R(0, 2)<1)
	{
		if (R(0, 2)>-1)
		{
			eulerxyz[1] = asin(R(0, 2));
			eulerxyz[0] = atan2(-R(1, 2), R(2, 2));
			eulerxyz[2] = atan2(-R(0, 1), R(0, 0));
		}
		else
		{
			eulerxyz[1] = -CV_PI / 2;
			eulerxyz[0] = -atan2(R(1, 0), R(1, 1));
			eulerxyz[2] = 0;
		}
	}
	else
	{
		eulerxyz[1] = CV_PI / 2;
		eulerxyz[0] = atan2(R(1, 0), R(1, 1));
		eulerxyz[2] = 0;
	}
	// eulerxyz[1] = eulerxyz[1] * 180 / CV_PI;
	// eulerxyz[0] = eulerxyz[0] * 180 / CV_PI;
	// eulerxyz[2] = eulerxyz[2] * 180 / CV_PI;
	return eulerxyz;
}

cv::Matx33f cvrm::euler2Rot(float x, float y, float z)
{
	cv::Matx33f R(1, 0, 0,
		0, 1, 0,
		0, 0, 1);

	// Assuming the angles are in radians.
	float cx = cos(x);
	float sx = sin(x);
	float cy = cos(y);
	float sy = sin(y);
	float cz = cos(z);
	float sz = sin(z);

	R(0, 0) = cy*cz;
	R(0,1) = -cy*sz;
	R(0,2) = sy;
	R(1,0) = cz*sx*sy + cx*sz;
	R(1,1) = cx*cz - sx*sy*sz;
	R(1,2) = -cy*sx;
	R(2,0) = -cx*cz*sy + sx*sz;
	R(2,1) = cz*sx + cx*sy*sz;
	R(2,2) = cx*cy;

	return R;
}


struct CompareSmallerVector3f {
	bool operator()(const Vec3f& v1,
		const Vec3f& v2) const {
		return v1[0] < v2[0] || (v1[0] == v2[0] && v1[1] < v2[1]) ||
			(v1[0] == v2[0] && v1[1] == v2[1] && v1[2] < v2[2]);
	}
};

inline void _subdivideTriangle(const Vec3f& v1, const Vec3f& v2, const Vec3f& v3, int n_divides, std::set<Vec3f, CompareSmallerVector3f>& points)
{
	if (n_divides == 0)
	{
		points.insert(v1);
		points.insert(v2);
		points.insert(v3);
	}
	else
	{
		Vec3f v12 = normalize(v1 + v2);
		Vec3f v13 = normalize(v1 + v3);
		Vec3f v23 = normalize(v2 + v3);
		_subdivideTriangle(v1, v12, v13, n_divides - 1, points);
		_subdivideTriangle(v2, v12, v23, n_divides - 1, points);
		_subdivideTriangle(v3, v13, v23, n_divides - 1, points);
		_subdivideTriangle(v12, v13, v23, n_divides - 1, points);
	}
}
inline std::vector<Vec3f> generateGeodesicPoints(int n_divides = 4)
{
	// Define icosahedron
	constexpr float x = 0.525731112119133606f;
	constexpr float z = 0.850650808352039932f;
	std::vector<Vec3f> icosahedron_points{
		{-x, 0.0f, z}, {x, 0.0f, z},  {-x, 0.0f, -z}, {x, 0.0f, -z},
		{0.0f, z, x},  {0.0f, z, -x}, {0.0f, -z, x},  {0.0f, -z, -x},
		{z, x, 0.0f},  {-z, x, 0.0f}, {z, -x, 0.0f},  {-z, -x, 0.0f} };
	std::vector<std::array<int, 3>> icosahedron_ids{
		{0, 4, 1},  {0, 9, 4},  {9, 5, 4},  {4, 5, 8},  {4, 8, 1},
		{8, 10, 1}, {8, 3, 10}, {5, 3, 8},  {5, 2, 3},  {2, 7, 3},
		{7, 10, 3}, {7, 6, 10}, {7, 11, 6}, {11, 0, 6}, {0, 1, 6},
		{6, 1, 10}, {9, 0, 11}, {9, 11, 2}, {9, 2, 5},  {7, 2, 11} };

	std::set < Vec3f, CompareSmallerVector3f> points;
	for (const auto& icosahedron_id : icosahedron_ids)
	{
		_subdivideTriangle(icosahedron_points[icosahedron_id[0]],
			icosahedron_points[icosahedron_id[1]],
			icosahedron_points[icosahedron_id[2]], n_divides, points);
	}

	return std::vector<Vec3f>(points.begin(), points.end());
}

void  cvrm::sampleSphereFromIcosahedron(std::vector<cv::Vec3f>& vecs, int timesOfSubdiv)
{
	auto v = generateGeodesicPoints(timesOfSubdiv);
	vecs.swap(v);
}


_CVR_API cv::Vec3f operator*(const cv::Vec3f &v, const Matx44f &M)
{
	Matx14f _p = Matx14f(v[0],v[1],v[2],1) * M;
	const float *p = _p.val;
	float s = 1.0f / p[3];
	return Vec3f(p[0] * s, p[1] * s, p[2] * s);
}

_CVR_API cv::Point3f operator*(const cv::Point3f &x, const Matx44f &M)
{
	Matx14f _p = Matx14f(x.x, x.y, x.z, 1) * M;
	const float *p = _p.val;
	float s = 1.0f / p[3];
	return Point3f(p[0] * s, p[1] * s, p[2] * s);
}

//==============================================================================================

class _CVRTrackBall
{
public:
	glm::vec3 r_axis_local = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::mat4 R = glm::mat4(1.0);
	glm::mat4 MV, Tr;
	int oldX = 0, oldY = 0;
	float r_angle = 0, dist = 0;

	float viewU = 0, viewV = 0;
public:
	glm::vec2 scaleMouse(glm::vec2 coords, glm::vec2 viewport) {
		return glm::vec2(static_cast<float>(coords.x*2.f) / static_cast<float>(viewport.x) - 1.f,
			1.f - static_cast<float>(coords.y*2.f) / static_cast<float>(viewport.y));
	}

	glm::vec3 projectToSphere(glm::vec2 xy) {
		static const float sqrt2 = sqrtf(2.f);
		glm::vec3 result;
		float d = glm::length(xy);
		float size_ = 2;
		if (d < size_ * sqrt2 / 2.f) {
			// Inside sphere
			// The factor "sqrt2/2.f" make a smooth changeover from sphere to hyperbola. If we leave
			// factor 1/sqrt(2) away, the trackball would bounce at the changeover.
			result.z = sqrtf(size_ * size_ - d*d);
		}
		else {
			// On hyperbola
			float t = size_ / sqrt2;
			result.z = t*t / d;
		}
		result.x = xy.x;
		result.y = xy.y;
		return glm::normalize(result);
	}

	void SetRotateParameter(glm::vec2 newMouse, glm::vec2 oldMouse)
	{
		if (newMouse == oldMouse) {
			// Zero rotation -> do nothing
			return;
		}

		// First, figure out z-coordinates for projection of P1 and P2 to deformed sphere
		glm::vec3 p1 = projectToSphere(oldMouse);
		glm::vec3 p2 = projectToSphere(newMouse);
		glm::vec3 r_axis_world = glm::cross(p1, p2);
		glm::vec3 d = p1 - p2;
		r_angle = 180 * glm::length(d) / 30;

		//transform rotate axis from world coordinate to local coordinate
		glm::vec3 r_axis_local_end = glm::vec3(glm::inverse(MV)*glm::vec4(r_axis_world, 1));
		glm::vec3 r_axis_local_start = glm::vec3(glm::inverse(MV)*glm::vec4(0.0, 0.0, 0.0, 1));
		r_axis_local = r_axis_local_end - r_axis_local_start;
		R = glm::rotate(R, r_angle, r_axis_local);
	}
	void UpdateMatrix()
	{
		Tr = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, dist));
		MV = Tr*R;
	}
	void OnMouseDown(int x, int y)
	{
		oldX = x;
		oldY = y;
	}
	void OnMouseMove(int x, int y, Size viewSize)
	{
		{
			glm::vec2 newMouse = scaleMouse(glm::vec2(x, y), glm::vec2(viewSize.width, viewSize.height));
			glm::vec2 oldMouse = scaleMouse(glm::vec2(oldX, oldY), glm::vec2(viewSize.width, viewSize.height));
			SetRotateParameter(newMouse, oldMouse);
			UpdateMatrix();
		}
		oldX = x;
		oldY = y;
	}
	void OnMouseWheel(int x, int y, int val)
	{
		dist += (val > 0 ? 0.25f : -0.25f);
	}
	void OnKeyDown(int key, int flags)
	{
		const float delta = 0.1f;
		switch (key)
		{
		case '4': //left
			viewU -= delta;
			break;
		case '6'://right
			viewU += delta;
			break;
		case '8'://up
			viewV += delta;
			break;
		case '2'://down
			viewV -= delta;
			break;
		}
	}
	Matx44f getViewRotate()
	{
		float sinv = sin(viewV);
		Vec3f dir(cos(viewU)*sinv, sin(viewU)*sinv, cos(viewV));
		return cvrm::rotate(Vec3f(0, 0, 1), dir);
	}
};

CVRTrackBall::CVRTrackBall()
	:impl(new _CVRTrackBall)
{}
CVRTrackBall::~CVRTrackBall()
{
}
void CVRTrackBall::onMouseDown(int x, int y)
{
	impl->OnMouseDown( x, y);
}
void CVRTrackBall::onMouseMove(int x, int y, Size viewSize)
{
	impl->OnMouseMove(x, y, viewSize);
}
void CVRTrackBall::onMouseWheel(int x, int y, int val)
{
	impl->OnMouseWheel(x, y, val);
}
void CVRTrackBall::onKeyDown(int key, int flags)
{
	impl->OnKeyDown(key, flags);
}
void CVRTrackBall::apply(Matx44f &mModel, Matx44f &mView, bool update)
{
	if (!update)
	{
		memcpy(mModel.val, &impl->R[0][0], sizeof(float) * 16);
		mView = impl->getViewRotate()*cvrm::translate(0, 0, impl->dist);
	}
	else
	{
		Matx44f T;
		memcpy(T.val, &impl->R[0][0], sizeof(float) * 16);
		mModel = mModel*T;

		mView = mView*impl->getViewRotate()*cvrm::translate(0, 0, impl->dist);
	}
}
