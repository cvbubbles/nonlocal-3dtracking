#pragma once

#include"CVX/core.h"
using namespace cv;



inline Mat1b getRenderMask(const Mat1f& depth, float eps = 1e-6)
{
	Mat1b mask = Mat1b::zeros(depth.size());
	for_each_2(DWHN1(depth), DN1(mask), [eps](float d, uchar& m) {
		m = fabs(1.0f - d) < eps ? 0 : 255;
		});
	return mask;
}

inline Vec3f viewDirFromRt(const Matx33f& R, const Vec3f& t, const Vec3f &modelCenter)
{
	return normalize(-R.inv() * t - modelCenter);
}
inline Vec3f viewDirFromR(const Matx33f& R)
{
	return normalize(Vec3f(R(2, 0), R(2, 1), R(2, 2)));
}

inline Vec2f dir2uv(const Vec3f& dir)
{
	float v = acos(dir[2]);
	float u = atan2(dir[1], dir[0]);
	if (u < 0)
		u += CV_PI * 2;
	return Vec2f(u, v);
}
inline Vec3f uv2dir(double u, double v)
{
	return Vec3f(cos(u) * sin(v), sin(u) * sin(v), cos(v));
}


inline cv::Matx33f uv2R(const Vec2f& uv)
{
	float cosv = cos(uv[1]), sinv = sin(uv[1]);
	Matx33f R(
		cosv, 0, sinv,
		0, 1, 0,
		-sinv, 0, cosv
	);

	cosv = cos(uv[0]), sinv = sin(uv[0]);
	return Matx33f(
		cosv, -sinv, 0,
		sinv, cosv, 0,
		0, 0, 1
	) * R;
}

inline cv::Matx33f rotateZ(float theta_x, float theta_y)
{
	float cosv = cos(theta_x), sinv = sin(theta_x);
	Matx33f R(
		1, 0, 0,
		0, cosv, -sinv,
		0, sinv, cosv
	);
	
	cosv = cos(theta_y); sinv = sin(theta_y);
	return Matx33f(
		cosv, 0, sinv,
		0, 1, 0,
		-sinv, 0, cosv
	) * R;
}

//inline Vec3f theta2dir(float theta_x, float theta_y)
//{
//	float cosx = cos(theta_x);
//
//	return Vec3f(sin(theta_y)*cosx, -sin(theta_x), cosx*cos(theta_y));
//}

inline Vec2f dir2theta(const Vec3f& dir)
{
	float theta_x = asin(-dir[1]);
	float cosx = sqrt(1 - dir[1] * dir[1]);
	float theta_y = fabs(cosx)<1e-6? 0.f : atan2(dir[0] / cosx, dir[2] / cosx);
	return Vec2f(theta_x, theta_y);
}


inline Matx33f eulerAnglesToRotationMatrix(const Vec3f &theta)
{
  Matx33f R_x (

             1,       0,              0,

             0,       cos(theta[0]),   -sin(theta[0]),

             0,       sin(theta[0]),   cos(theta[0])

             );

  // Calculate rotation about y axis

  Matx33f R_y(

             cos(theta[1]),    0,      sin(theta[1]),

             0,               1,      0,

             -sin(theta[1]),   0,      cos(theta[1])

             );

  // Calculate rotation about z axis

  Matx33f R_z (

             cos(theta[2]),    -sin(theta[2]),      0,

             sin(theta[2]),    cos(theta[2]),       0,

             0,               0,                  1);



  // Combined rotation matrix

  return R_z * R_y * R_x;
}

// Checks if a matrix is a valid rotation matrix.
inline bool isRotationMatrix(Mat R)
{

 Mat Rt;

 transpose(R, Rt);

 Mat shouldBeIdentity = Rt * R;

 Mat I = Mat::eye(3,3, shouldBeIdentity.type());

 return  norm(I, shouldBeIdentity) < 1e-6;
}

inline Vec3f rotationMatrixToEulerAngles(Matx33f R)
{
  //  assert(isRotationMatrix(R));
    float sy = sqrt(R(0,0) * R(0,0) +  R(1,0) * R(1,0) );
    bool singular = sy < 1e-6; // If
    float x, y, z;
    if (!singular)
    {
        x = atan2(R(2,1) , R(2,2));
        y = atan2(-R(2,0), sy);
        z = atan2(R(1,0), R(0,0));
    }
  else
  {
      x = atan2(-R(1,2), R(1,1));

      y = atan2(-R(2,0), sy);

      z = 0;
  }
  return Vec3f(x, y, z);
}

inline cv::Matx33f getRFromGLM(const Matx44f& m)
{
	const float* v = m.val;

	return  cv::Matx33f(v[0], v[4], v[8], v[1], v[5], v[9], v[2], v[6], v[10]);
}

inline cv::Matx33f rotationAroundAxis(const Vec3f& axis, float angle)
{
	auto m = cvrm::rotate(angle, axis);
	return getRFromGLM(m);
}

inline cv::Matx33f dir2OutofplaneRotation(const Vec3f& dir)
{
	auto eyePos = dir;
	auto m = cvrm::lookat(eyePos[0], eyePos[1], eyePos[2], 0, 0, 0, 0, 1, 0);
	return getRFromGLM(m);
}

inline cv::Vec3f theta2Dir(float theta_x, float theta_y)
{
	CV_Assert(fabs(theta_x) < CV_PI / 2 || fabs(theta_y) < CV_PI / 2);

	if (fabs(theta_x) < 1e-6f)
	{
		return Vec3f(0.f, sin(theta_y), cos(theta_y));
	}
	else if (fabs(theta_y) < 1e-6f)
	{
		return Vec3f(sin(theta_x), 0.f, cos(theta_x));
	}

	float a = 1.f/tan(theta_x);
	float b = 1.f/tan(theta_y);
	float z = sqrt(1.f / (a * a + b * b + 1.f));

	return normalize(Vec3f(z/a, z/b, z));
}

inline cv::Vec2f dir2Theta(const Vec3f& dir)
{
	float theta_x = atan2(dir[0], dir[2]);
	float theta_y = atan2(dir[1], dir[2]);
	return Vec2f(theta_x, theta_y);
}

inline cv::Matx33f theta2OutofplaneRotation(float theta_x, float theta_y)
{
	return dir2OutofplaneRotation(theta2Dir(theta_x, theta_y));
}

inline void decomposeRinRout(const cv::Matx33f& R, cv::Matx33f& Rin, cv::Matx33f& Rout)
{
	Vec3f v= normalize(Vec3f(R(2, 0), R(2, 1), R(2, 2)));
	Rout = dir2OutofplaneRotation(v);
	Rin = R * Rout.t();
}
