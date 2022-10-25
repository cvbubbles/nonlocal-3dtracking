
#include"cvrender.h"
#include"opencv2/calib3d/calib3d.hpp"

class Camerax
{
public:
	Camerax();
	Camerax(float fx, float fy, float cx, float cy);
	Camerax(float fx, float fy, float cx, float cy, float distortion[5]);
	Camerax(const Camerax& cam);
	~Camerax();

	float getfx() const;
	float getfy() const;
	float getcx() const;
	float getcy() const;
	cv::Mat getIntrinsic() const;
	const float* getProjectionIntrinsic(float width, float height, float nearP, float farP);
	cv::Mat getExtrinsic() const;
	const float* getModelviewExtrinsic();
	cv::Mat getDistorsions() const;

	void setIntrinsic(float fx, float fy, float cx, float cy);
	void setIntrinsic(const cv::Mat& intrinsic);
	void setExtrinsic(float rx, float ry, float rz, float tx, float ty, float tz);
	void setExtrinsic(const cv::Mat& extrinsic);
	void setDistortion(float distortion[5]);
	void setDistortion(const cv::Mat& distortion);

	cv::Mat flipExtrinsicYZ() const; // convert the pipe coordinate system between opencv and opengl context 

	void copyFrom(const Camerax& cam);

private:
	cv::Mat m_intrinsic; // pipe intrinsic parameters 3*3
	cv::Mat m_distortion; // pipe distortion parameters 5*1
	cv::Mat m_extrinsic; // pipe extrinsic parameters 3*4

	float m_fx, m_fy, m_cx, m_cy;

	float *glProjectMatrix;
	float *glModelviewMatrix;
};

Camerax::Camerax()
	: m_intrinsic(3, 3, CV_32FC1), m_extrinsic(3, 4, CV_32FC1), m_distortion(5, 1, CV_32FC1),
	m_fx(0.0f), m_fy(0.0f), m_cx(0.0f), m_cy(0.0f)
{
	m_intrinsic = cv::Mat::zeros(3, 3, CV_32FC1);
	m_extrinsic = cv::Mat::zeros(3, 4, CV_32FC1);
	m_distortion = cv::Mat::zeros(5, 1, CV_32FC1);

	glProjectMatrix = (float*)malloc(16 * sizeof(float));
	glModelviewMatrix = (float*)malloc(16 * sizeof(float));
}

Camerax::Camerax(float fx, float fy, float cx, float cy)
	: m_intrinsic(3, 3, CV_32FC1), m_extrinsic(3, 4, CV_32FC1), m_distortion(5, 1, CV_32FC1),
	m_fx(fx), m_fy(fy), m_cx(cx), m_cy(cy)
{
	m_intrinsic = cv::Mat::zeros(3, 3, CV_32FC1);
	m_extrinsic = cv::Mat::zeros(3, 4, CV_32FC1);
	m_distortion = cv::Mat::zeros(5, 1, CV_32FC1);

	m_intrinsic.ptr<float>(0)[0] = fx; m_intrinsic.ptr<float>(0)[1] = 0.0f; m_intrinsic.ptr<float>(0)[2] = cx;
	m_intrinsic.ptr<float>(1)[0] = 0.0f; m_intrinsic.ptr<float>(1)[1] = fy; m_intrinsic.ptr<float>(1)[2] = cy;
	m_intrinsic.ptr<float>(2)[0] = 0.0f; m_intrinsic.ptr<float>(2)[1] = 0.0f; m_intrinsic.ptr<float>(2)[2] = 1.0f;

	glProjectMatrix = (float*)malloc(16 * sizeof(float));
	glModelviewMatrix = (float*)malloc(16 * sizeof(float));
}

Camerax::Camerax(float fx, float fy, float cx, float cy, float distortion[5])
	: m_intrinsic(3, 3, CV_32FC1), m_extrinsic(3, 4, CV_32FC1), m_distortion(5, 1, CV_32FC1),
	m_fx(fx), m_fy(fy), m_cx(cx), m_cy(cy)
{
	m_intrinsic = cv::Mat::zeros(3, 3, CV_32FC1);
	m_extrinsic = cv::Mat::zeros(3, 4, CV_32FC1);
	m_distortion = cv::Mat::zeros(5, 1, CV_32FC1);

	m_intrinsic.ptr<float>(0)[0] = fx; m_intrinsic.ptr<float>(0)[1] = 0.0f; m_intrinsic.ptr<float>(0)[2] = cx;
	m_intrinsic.ptr<float>(1)[0] = 0.0f; m_intrinsic.ptr<float>(1)[1] = fy; m_intrinsic.ptr<float>(1)[2] = cy;
	m_intrinsic.ptr<float>(2)[0] = 0.0f; m_intrinsic.ptr<float>(2)[1] = 0.0f; m_intrinsic.ptr<float>(2)[2] = 1.0f;

	m_distortion.ptr<float>(0)[0] = distortion[0]; m_distortion.ptr<float>(1)[0] = distortion[1];
	m_distortion.ptr<float>(2)[0] = distortion[2]; m_distortion.ptr<float>(3)[0] = distortion[3];
	m_distortion.ptr<float>(4)[0] = distortion[4];

	glProjectMatrix = (float*)malloc(16 * sizeof(float));
	glModelviewMatrix = (float*)malloc(16 * sizeof(float));
}

Camerax::Camerax(const Camerax& cam)
{
	m_intrinsic = cam.m_intrinsic.clone();
	m_extrinsic = cam.m_extrinsic.clone();
	m_distortion = cam.m_distortion.clone();

	m_fx = cam.m_fx;
	m_fy = cam.m_fy;
	m_cx = cam.m_cx;
	m_cy = cam.m_cy;

	glProjectMatrix = (float*)malloc(16 * sizeof(float));
	glModelviewMatrix = (float*)malloc(16 * sizeof(float));

	memcpy(glProjectMatrix, cam.glModelviewMatrix, 16 * sizeof(float));
	memcpy(glModelviewMatrix, cam.glModelviewMatrix, 16 * sizeof(float));
}

void Camerax::copyFrom(const Camerax& cam)
{
	m_intrinsic = cam.m_intrinsic.clone();
	m_extrinsic = cam.m_extrinsic.clone();
	m_distortion = cam.m_distortion.clone();

	m_fx = cam.m_fx;
	m_fy = cam.m_fy;
	m_cx = cam.m_cx;
	m_cy = cam.m_cy;

	memcpy(glProjectMatrix, cam.glProjectMatrix, 16 * sizeof(float));
	memcpy(glModelviewMatrix, cam.glModelviewMatrix, 16 * sizeof(float));
}

Camerax::~Camerax()
{
	free(glProjectMatrix);
	free(glModelviewMatrix);
}

float Camerax::getfx() const
{
	return m_fx;
}

float Camerax::getfy() const
{
	return m_fy;
}

float Camerax::getcx() const
{
	return m_cx;
}

float Camerax::getcy() const
{
	return m_cy;
}

cv::Mat Camerax::getIntrinsic() const
{
	return m_intrinsic;
}

const float* Camerax::getProjectionIntrinsic(float width, float height, float nearP, float farP)
{
	// in opengl context, the matrix is stored in column-major order 
	glProjectMatrix[0] = 2.0f * m_fx / width;
	glProjectMatrix[1] = 0.0f;
	glProjectMatrix[2] = 0.0f;
	glProjectMatrix[3] = 0.0f;

	glProjectMatrix[4] = 0.0f;
	glProjectMatrix[5] = 2.0f * m_fy / height;
	glProjectMatrix[6] = 0.0f;
	glProjectMatrix[7] = 0.0f;


	glProjectMatrix[8] = 2.0f * m_cx / width - 1.0f;
	glProjectMatrix[9] = 2.0f * m_cy / height - 1.0f;
	glProjectMatrix[10] = -(farP + nearP) / (farP - nearP);
	glProjectMatrix[11] = -1.0f;


	glProjectMatrix[12] = 0.0f;
	glProjectMatrix[13] = 0.0f;
	glProjectMatrix[14] = -2.0f * farP * nearP / (farP - nearP);
	glProjectMatrix[15] = 0.0f;

	return glProjectMatrix;
}

cv::Mat Camerax::getExtrinsic() const
{
	return m_extrinsic;
}

const float* Camerax::getModelviewExtrinsic()
{
	// from opencv context to opengl context
	// we need rotate the object frame along X with 180 degrees
	cv::Mat flipedExtrinsic = flipExtrinsicYZ();

	// in opengl context, the matrix is stored in column-major order 
	glModelviewMatrix[0] = flipedExtrinsic.ptr<float>(0)[0];
	glModelviewMatrix[1] = flipedExtrinsic.ptr<float>(1)[0];
	glModelviewMatrix[2] = flipedExtrinsic.ptr<float>(2)[0];
	glModelviewMatrix[3] = 0.0f;

	glModelviewMatrix[4] = flipedExtrinsic.ptr<float>(0)[1];
	glModelviewMatrix[5] = flipedExtrinsic.ptr<float>(1)[1];
	glModelviewMatrix[6] = flipedExtrinsic.ptr<float>(2)[1];
	glModelviewMatrix[7] = 0.0f;


	glModelviewMatrix[8] = flipedExtrinsic.ptr<float>(0)[2];
	glModelviewMatrix[9] = flipedExtrinsic.ptr<float>(1)[2];
	glModelviewMatrix[10] = flipedExtrinsic.ptr<float>(2)[2];
	glModelviewMatrix[11] = 0.0f;


	glModelviewMatrix[12] = flipedExtrinsic.ptr<float>(0)[3];
	glModelviewMatrix[13] = flipedExtrinsic.ptr<float>(1)[3];
	glModelviewMatrix[14] = flipedExtrinsic.ptr<float>(2)[3];
	glModelviewMatrix[15] = 1.0f;

	return glModelviewMatrix;
}

cv::Mat Camerax::getDistorsions() const
{
	return m_distortion;
}

void Camerax::setIntrinsic(float fx, float fy, float cx, float cy)
{
	m_fx = fx;
	m_fy = fy;
	m_cx = cx;
	m_cy = cy;
	m_intrinsic.ptr<float>(0)[0] = fx; m_intrinsic.ptr<float>(0)[1] = 0.0f; m_intrinsic.ptr<float>(0)[2] = cx;
	m_intrinsic.ptr<float>(1)[0] = 0.0f; m_intrinsic.ptr<float>(1)[1] = fy; m_intrinsic.ptr<float>(1)[2] = cy;
	m_intrinsic.ptr<float>(2)[0] = 0.0f; m_intrinsic.ptr<float>(2)[1] = 0.0f; m_intrinsic.ptr<float>(2)[2] = 1.0f;
}

void Camerax::setIntrinsic(const cv::Mat& intrinsic)
{
	m_intrinsic = intrinsic.clone();
	m_fx = m_intrinsic.ptr<float>(0)[0];
	m_fy = m_intrinsic.ptr<float>(1)[1];
	m_cx = m_intrinsic.ptr<float>(0)[2];
	m_cy = m_intrinsic.ptr<float>(1)[2];
}

void Camerax::setExtrinsic(float rx, float ry, float rz, float tx, float ty, float tz)
{
	cv::Mat rvmat = cv::Mat::zeros(1, 3, CV_32FC1);
	rvmat.ptr<float>(0)[0] = rx; rvmat.ptr<float>(0)[1] = ry; rvmat.ptr<float>(0)[2] = rz;
	cv::Mat rmat = cv::Mat::zeros(3, 3, CV_32FC1);
	cv::Rodrigues(rvmat, rmat);

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
}

void Camerax::setExtrinsic(const cv::Mat& extrinsic)
{
	m_extrinsic = extrinsic.clone();
}

void Camerax::setDistortion(float distortion[5])
{
	m_distortion.ptr<float>(0)[0] = distortion[0]; m_distortion.ptr<float>(1)[0] = distortion[1];
	m_distortion.ptr<float>(2)[0] = distortion[2]; m_distortion.ptr<float>(3)[3] = distortion[3];
	m_distortion.ptr<float>(4)[0] = distortion[4];
}
void Camerax::setDistortion(const cv::Mat& distortion)
{
	m_distortion = distortion.clone();
}

cv::Mat Camerax::flipExtrinsicYZ() const
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


void CVRPipe::setProjection(float fx, float fy, float cx, float cy, Size windowSize, float nearP, float farP)
{
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


	mProjection[8] = 2.0f * cx / width - 1.0f;
	mProjection[9] = 2.0f * cy / height - 1.0f;
	mProjection[10] = -(farP + nearP) / (farP - nearP);
	mProjection[11] = -1.0f;


	mProjection[12] = 0.0f;
	mProjection[13] = 0.0f;
	mProjection[14] = -2.0f * farP * nearP / (farP - nearP);
	mProjection[15] = 0.0f;
}

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

void CVRPipe::setModelview(float rx, float ry, float rz, float tx, float ty, float tz)
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
}

