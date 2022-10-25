
#include"cvrender.h"

//#include"GL/glut.h"
#include"glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include"opencv2/calib3d.hpp"
using namespace cv;

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

Matx44f cvrm::perspective(float f, Size windowSize, float nearP, float farP)
{
	auto m=glm::perspectiveFov<float>(2 * atan2(windowSize.height / 2.0f, f), windowSize.width, windowSize.height, nearP, farP);
	return cvtm(m);
}

Matx44f cvrm::perspective(float fx, float fy, float cx, float cy, Size windowSize, float nearP, float farP)
{
	Matx44f m;
	float *mProjection = m.val;

	const int width = windowSize.width, height = windowSize.height;

	// in opengl context, the matrix is stored in column-major order 
	mProjection[0] = 2.0f * fx / width;
	mProjection[1] = 0.0f;
	mProjection[2] = 0.0f;
	mProjection[3] = 0.0f;

	mProjection[4] = 0.0f;
	mProjection[5] = 2.0f * fy / height;
	mProjection[6] = 0.0f;
	mProjection[7] = 0.0f;


	mProjection[8] = 1 - 2.0f * cx / width;
	mProjection[9] = 2.0f * cy / height - 1.0f;
	mProjection[10] = -(farP + nearP) / (farP - nearP);
	mProjection[11] = -1.0f;


	mProjection[12] = 0.0f;
	mProjection[13] = 0.0f;
	mProjection[14] = -2.0f * farP * nearP / (farP - nearP);
	mProjection[15] = 0.0f;

	return m;
}

Matx44f cvrm::fromK(const cv::Mat &K, Size windowSize, float nearP, float farP)
{
	cv::Mat1f Kf(K);
	return perspective(Kf(0, 0), Kf(1, 1), Kf(0, 2), Kf(1, 2), windowSize, nearP, farP);
}

Matx44f cvrm::lookat(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz)
{
	auto m = glm::lookAt(glm::vec3(eyex, eyey, eyez), glm::vec3(centerx, centery, centerz), glm::vec3(upx, upy, upz));
	return cvtm(m);
}

static void extrinsicMatrix2ModelViewMatrix(cv::Mat rotation, cv::Mat translation, double* model_view_matrix)
{
	//绕X轴旋转180度，从OpenCV坐标系变换为OpenGL坐标系
	static double d[] =
	{
		1,  0,  0,
		0, -1,  0,
		0,  0, -1
	};
	cv::Mat_<double> rx(3, 3, d);

	rotation = rx*rotation;
	translation = rx*translation;

	//opengl默认的矩阵为列主序
	model_view_matrix[0] = rotation.at<double>(0, 0);
	model_view_matrix[1] = rotation.at<double>(1, 0);
	model_view_matrix[2] = rotation.at<double>(2, 0);
	model_view_matrix[3] = 0.0f;

	model_view_matrix[4] = rotation.at<double>(0, 1);
	model_view_matrix[5] = rotation.at<double>(1, 1);
	model_view_matrix[6] = rotation.at<double>(2, 1);
	model_view_matrix[7] = 0.0f;

	model_view_matrix[8] = rotation.at<double>(0, 2);
	model_view_matrix[9] = rotation.at<double>(1, 2);
	model_view_matrix[10] = rotation.at<double>(2, 2);
	model_view_matrix[11] = 0.0f;

	model_view_matrix[12] = translation.at<double>(0, 0);
	model_view_matrix[13] = translation.at<double>(1, 0);
	model_view_matrix[14] = translation.at<double>(2, 0);
	model_view_matrix[15] = 1.0f;
}
Matx44f cvrm::fromRT(const cv::Mat &rvec, const cv::Mat &tvec)
{
	cv::Mat R;
	cv::Rodrigues(rvec, R);

	cv::Matx44d m;
	extrinsicMatrix2ModelViewMatrix(R, tvec, m.val);
	return Matx44f(m);
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
void CVRTrackBall::apply(Matx44f &mModel, Matx44f &mView, bool update)
{
	if (!update)
	{
		memcpy(mModel.val, &impl->R[0][0], sizeof(float) * 16);
		mView = cvrm::translate(0, 0, impl->dist);
	}
	else
	{
		Matx44f T;
		memcpy(T.val, &impl->R[0][0], sizeof(float) * 16);
		mModel = mModel*T;

		mView = mView*cvrm::translate(0, 0, impl->dist);
	}
}
