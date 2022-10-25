#pragma once

#ifdef CVRENDER_STATIC
#define _CVR_API
#endif

#ifndef _CVR_API

#if defined(_WIN32)

#ifdef CVRENDER_EXPORTS
#define _CVR_API __declspec(dllexport)
#else
#define _CVR_API __declspec(dllimport)
#endif

#elif defined(__GNUC__) && __GNUC__ >= 4

#ifdef CVRENDER_EXPORTS
#define _CVR_API __attribute__ ((visibility ("default")))
#else
#define _CVR_API 
#endif

#elif
#define _CVR_API
#endif
#endif

#ifdef _MSC_VER

#pragma warning(disable:4267)
#pragma warning(disable:4251)
#pragma warning(disable:4190)
#pragma warning(disable:4819)

#endif


#include"opencv2/core/core.hpp"
#include<memory>

using cv::Matx44f;
using cv::Matx33f;
using cv::Size;

_CVR_API void cvrInit(const char *args = NULL, void *atexitFP=std::atexit);
//_CVR_API void cvrUninit();

class _CVR_API cvrm
{
public:
	static Matx44f diag(float s);

	static Matx44f I()
	{
		return diag(1.0f);
	}

	static Matx44f translate(float tx, float ty, float tz);

	static Matx44f scale(float sx, float sy, float sz);

	static Matx44f scale(float s)
	{
		return scale(s, s, s);
	}

	static Matx44f rotate(float angle, const cv::Vec3f &axis);

	//rotation M that transforms s to t : t=s*M;
	static Matx44f  rotate(const cv::Vec3f &s, const cv::Vec3f &t);

	static Matx44f rotate(const float r0[3], const float r1[3], const float r2[3]);

	static Matx44f ortho(float left, float right, float bottom, float top, float nearP, float farP);

	static Matx44f perspective(float f, Size windowSize, float nearP, float farP);

	//conver intrinsic parameters to projection matrix
	static Matx44f perspective(float fx, float fy, float cx, float cy, Size windowSize, float nearP, float farP);

	//fscale = focal-length/image-height;
	static Matx33f defaultK(Size imageSize, float fscale);

	static Matx33f scaleK(const Matx33f& K, float scalex, float scaley);

	static Matx33f scaleK(const Matx33f& K, float scale)
	{
		return scaleK(K, scale, scale);
	}

	static Matx44f fromK(const Matx33f &K, Size windowSize, float nearP, float farP);

	//convert rotation and translation vectors to model-view matrix
	//Note: @rvec and @tvec are in OpenCV coordinates, returned 4x4 matrix is in OpenGL coordinates
	static Matx44f fromRT(const cv::Vec3f &rvec, const cv::Vec3f &tvec);

	static Matx44f fromR33T(const cv::Matx33f &R, const cv::Vec3f &tvec);

	//decompose a 3x4 (OpenGL) transformation matrix to (OpenCV) Rotation-Translation vectors
	//the matrix @m should contain only rotation and translation
	static void decomposeRT(const Matx44f &m, cv::Vec3f &rvec, cv::Vec3f &tvec);

	static void decomposeRT(const Matx44f &m, cv::Matx33f &R, cv::Vec3f &tvec);

	static Matx44f lookat(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz);

	static cv::Point3f unproject(const cv::Point3f &winPt, const Matx44f &mModelview, const Matx44f &mProjection, const int viewport[4]);

	static cv::Point3f project(const cv::Point3f &objPt, const Matx44f &mModelview, const Matx44f &mProjection, const int viewport[4]);

	static void  sampleSphere(std::vector<cv::Vec3f> &vecs, int n);

	static cv::Vec3f rot2Euler(const cv::Matx33f &R);

	static cv::Matx33f euler2Rot(float rx, float ry, float rz);
	
	/*
	    timesOfSubdiv=0, nPoints=12
		timesOfSubdiv=1, nPoints=42
		timesOfSubdiv=2, nPoints=162
		timesOfSubdiv=3, nPoints=642
		timesOfSubdiv=4, nPoints=2562
		timesOfSubdiv=5, nPoints=10242
		timesOfSubdiv=6, nPoints=40962
		timesOfSubdiv=7, nPoints=163842
		timesOfSubdiv=8, nPoints=655362
	*/
	static void  sampleSphereFromIcosahedron(std::vector<cv::Vec3f>& vecs, int timesOfSubdiv=4);
};

// y=x*M;
_CVR_API cv::Vec3f operator*(const cv::Vec3f &x, const Matx44f &M);

_CVR_API cv::Point3f operator*(const cv::Point3f &x, const Matx44f &M);

class _CVR_API CVRRendable
{
private:
	bool  _visible = true;
public:
	virtual void render(const Matx44f &sceneModelView, int flags) = 0;

	virtual void setVisible(bool visible);

	bool  isVisible() const
	{
		return _visible;
	}

	virtual ~CVRRendable();
};

class _CVR_API CVRMesh
{
public:
	std::vector<char>           verticesMask;
	std::vector<cv::Vec3f>		vertices;
	std::vector<cv::Vec3f>      normals;

	struct Faces
	{
		int  numVertices;
		std::vector<int>   indices;
	};

	std::vector<Faces>		faces;
	std::vector<std::string>   texFiles;
public:
	void clear()
	{
		verticesMask.clear();
		vertices.clear();
		normals.clear();
		faces.clear();
		texFiles.clear();
	}
};

struct _CVRModel;

class _CVR_API CVRModel
	:public CVRRendable
{
	friend class _CVRender;
	std::shared_ptr<_CVRModel> _model;
public:
	virtual void render(const Matx44f &sceneModelView, int flags);
public:
	CVRModel();
	
	CVRModel(const std::string &file);

	~CVRModel();

	void clear();

	bool empty() const;

	operator bool() const
	{
		return !empty();
	}

	/* load model file, the supported formats include .obj, .3ds, .ply, .stl, .dae ... (see https://github.com/assimp/assimp)

	The @postProLevel (0-3) specifies the post-processing levels for optimizing the mesh for rendering. 0 means no post processing.
	*/
	void load(const std::string &file, int postProLevel=3, const std::string &options="");

	/* save the model to a file, any file format that supported by Assimp can be used, such as .obj, .3ds, .ply, .stl, .dae ,...
	 @fmtID : a string to specify the file format, the file extension will be used if is empty.
	 @options : additional options, use "-std" to rename the texture image files in a standard way.
	*/
	void saveAs(const std::string &file, const std::string &fmtID = "", const std::string &options="-std");

	//get a transformation that transform the model to a standard position and size (centered at the origin and scale to fit in a unit cube)
	static Matx44f getUnitize(const cv::Vec3f &center, const cv::Vec3f &bbMin, const cv::Vec3f &bbMax);

	Matx44f getUnitize() const;

	cv::Vec3f getCenter() const;

	const std::vector<cv::Vec3f>& getVertices() const;

	void  getMesh(CVRMesh &mesh, int flags = 0);

	void    getBoundingBox(cv::Vec3f &cMin, cv::Vec3f &cMax) const;

	cv::Vec<float,6> __getBoundingBox() const
	{
		cv::Vec3f bb[2];
		this->getBoundingBox(bb[0], bb[1]);
		return reinterpret_cast<cv::Vec<float,6>&>(bb);
	}

	std::vector<cv::Point3f>  getBoundingBoxCorners() const
	{
		using cv::Point3f;
		cv::Vec3f cMin, cMax;
		this->getBoundingBox(cMin, cMax);
		return {
			Point3f(cMin[0],cMin[1],cMin[2]), Point3f(cMax[0],cMin[1],cMin[2]), Point3f(cMax[0],cMax[1],cMin[2]), Point3f(cMin[0],cMax[1],cMin[2]),
			Point3f(cMin[0],cMin[1],cMax[2]), Point3f(cMax[0],cMin[1],cMax[2]), Point3f(cMax[0],cMax[1],cMax[2]), Point3f(cMin[0],cMax[1],cMax[2]),
		};
	}

	//get size of bounding box
	cv::Vec3f  getSizeBB() const ;

	const std::string& getFile() const;

	/*set a transformation to the model that place it in a standard way
	  this transformation will be applied before the transformations in CVRMats 
	  i.e., it acts as a change to the coordinates of model vertices.
	*/
	void    transform(const Matx44f &trans);

	/*get the current Pose0
	note that @load also may set Pose0 if the option "-setPose0" is specified
	*/
	//Matx44f getPose0() const;

	/*estimate Pose0 that shift object center to the origin, and rotate object that PCA eigen vectors aligns with axises.
	  (with y-axis the longest dimension, x-axis the second longest, and z-axis the shortest)
	*/
	Matx44f estimatePose0() const;
};

/* The matrixs related with OpenGL rendering, a 3D point X is transformed to an image point x by: x=X'*mModeli*mModel*mView*mProjection
*/
class _CVR_API CVRMats
{
public:
	Matx44f mModeli;      //transform the model to a standard state (postion, pose, size)
	Matx44f mModel;       //motion of the model
	Matx44f mView;        //motion of the camera
	Matx44f mProjection;  //projection from 3D to 2D (the projection matrix)
public:
	//initialize all matrix with mInit
	CVRMats(const Matx44f &mInit=cvrm::I());

	//set mView and mProjection
	//focalLength=fscale*viewHeight, fscale=sqrt(2)
	CVRMats(Size viewSize, float fscale = 1.5f, float eyeDist = 4.0f, float zNear = 0.1f, float zFar = 100.f);

	//set the matrixs to show a model in a standard way (used in mdshow(...) )
	CVRMats(const CVRModel &model, Size viewSize, float fscale = 1.5f, float eyeDist = 4.0f, float zNear = 0.1f, float zFar = 100.f);

	void setUtilizedModelView(const CVRModel &model, float eyeDist = 4.0f);

	//set Mats to show the model in an image or in an image ROI
	//sizeScale : scale of the object size in image space (by adjust eyeDist)
	//zNear,zFar: <=0 if need to be auto estimated
	void setInImage(const CVRModel &model, Size viewSize, const Matx33f &K, float sizeScale = 1.0f, float zNear=-1.f, float zFar=-1.f);

	void setInROI(const CVRModel &model, Size viewSize, cv::Rect roi, const Matx33f &K, float sizeScale = 1.0f, float zNear=-1.f, float zFar=-1.f);

	Matx44f modelView() const
	{
		return mModeli*mModel*mView;
	}
};

class _CVR_API CVRResult
{
public:
	cv::Mat  img;
	cv::Mat1f  depth;
	CVRMats    mats;
	cv::Rect   outRect;
public:
	void getDepthRange(float &minDepth, float &maxDepth) const;

	float getDepthRange() const;

	cv::Mat1f getNormalizedDepth() const;

	bool  getDepth(float x, float y, float &d) const;
	
	bool  isVisible(const cv::Point3f &pt, float depthDelta=1e-3f)
	{
		float dz;
		return this->getDepth(pt.x, pt.y, dz) && pt.z < dz + depthDelta;
	}

	cv::Mat1b getMaskFromDepth(float eps = 1e-6f);

	static CVRResult blank(Size viewSize, const CVRMats &_mats);
};

class _CVR_API CVRProjector
{
	cv::Matx44f _mModelView;
	cv::Matx44f _mProjection;
	cv::Vec4i   _viewport;
	
	cv::Mat1f   _depth;
	cv::Point2f _depthOffset;
public:
	CVRProjector();

	CVRProjector(const CVRResult &rr, cv::Size viewSize=cv::Size(0,0));

	CVRProjector(const CVRMats &mats, cv::Size viewSize);

	CVRProjector(const cv::Matx44f &mModelView, const cv::Matx44f &mProjection, cv::Size viewSize);

	CVRProjector(const cv::Matx44f &mModelView, const cv::Matx33f &cameraK, cv::Size viewSize, float nearP = 1.0f, float farP = 100.0f);

	//project 3D point to 2D
	cv::Point3f project(float x, float y, float z) const
	{
		cv::Point3f p = cvrm::project(cv::Point3f(x, y, z), _mModelView, _mProjection, &_viewport[0]);
		p.y = _viewport[3] - p.y;
		return p;
	}
	cv::Point3f project(const cv::Point3f &pt) const
	{
		return project(pt.x, pt.y, pt.z);
	}
	void  project(const cv::Point3f vpt[], cv::Point3f dpt[], int count) const
	{
		for (int i = 0; i < count; ++i)
			dpt[i] = project(vpt[i]);
	}
	void project(const std::vector<cv::Point3f> &vpt, std::vector<cv::Point3f> &dpt) const
	{
		dpt.resize(vpt.size());
		if (!vpt.empty())
			project(&vpt[0], &dpt[0], (int)vpt.size());
	}
	void  project(const cv::Point3f vpt[], cv::Point2f dpt[], int count) const
	{
		for (int i = 0; i < count; ++i)
		{
			cv::Point3f ptx= project(vpt[i]);
			dpt[i] = cv::Point2f(ptx.x, ptx.y);
		}
	}
	void project(const std::vector<cv::Point3f> &vpt, std::vector<cv::Point2f> &dpt) const
	{
		dpt.resize(vpt.size());
		if (!vpt.empty())
			project(&vpt[0], &dpt[0], (int)vpt.size());
	}
	void project(const std::vector<cv::Point3f> &vpt, std::vector<cv::Point> &dpt) const
	{
		dpt.resize(vpt.size());
		for (size_t i = 0; i < vpt.size(); ++i)
		{
			cv::Point3f ptx = project(vpt[i]);
			dpt[i] = cv::Point(int(ptx.x + 0.5), int(ptx.y + 0.5));
		}
	}
	//unproject a 2D point (with depth) to 3D
	cv::Point3f unproject(float x, float y, float depth) const
	{
		return cvrm::unproject(cv::Point3f(x, _viewport[3] - y, depth), _mModelView, _mProjection, &_viewport[0]);
	}

	cv::Point3f unproject(float x, float y) const;
	
	cv::Point3f unproject(const cv::Point2f &pt) const
	{
		return unproject(pt.x, pt.y);
	}
	void  unproject(const cv::Point2f vpt[], cv::Point3f dpt[], int count) const
	{
		for (int i = 0; i < count; ++i)
			dpt[i] = unproject(vpt[i]);
	}
	void unproject(const std::vector<cv::Point2f> &vpt, std::vector<cv::Point3f> &dpt) const
	{
		dpt.resize(vpt.size());
		if (!vpt.empty())
			unproject(&vpt[0], &dpt[0], (int)vpt.size());
	}
};

enum
{
	CVRM_ENABLE_LIGHTING=0x01,
	CVRM_ENABLE_TEXTURE=0x02,
	CVRM_ENABLE_MATERIAL=0x04,
	CVRM_TEXTURE_NOLIGHTING=0x08,
	
	//rendering with default lighting, texture and material
	CVRM_ENABLE_ALL= CVRM_ENABLE_LIGHTING| CVRM_ENABLE_TEXTURE | CVRM_ENABLE_MATERIAL,

	//for object with textures, rendering texture color without lighting effect.
	CVRM_TEXCOLOR= CVRM_ENABLE_ALL | CVRM_TEXTURE_NOLIGHTING,

	CVRM_DEFAULT=CVRM_TEXCOLOR 
};

enum
{
	CVRM_IMAGE=0x01,
	CVRM_DEPTH=0x02,
//	CVRM_ALPHA=0x04
//	CVRM_NO_VFLIP = 0x10,
	CVRM_FLIP = 0x10,
};

template<typename _ValT>
class CVRArray
	:public CVRRendable
{
protected:
	typedef std::vector<_ValT> _CtrT;

	_CtrT  _v;
public:
	typedef _ValT value_type;
	typedef typename _CtrT::iterator iterator;
	typedef typename _CtrT::const_iterator const_iterator;
public:
	CVRArray()
	{}
	CVRArray(size_t size)
		:_v(size)
	{
	}
	void resize(size_t size)
	{
		_v.resize(size);
	}
	void push_back(const _ValT &val)
	{
		_v.push_back(val);
	}
	size_t size() const
	{
		return _v.size();
	}
	_ValT& operator[](int i)
	{
		return _v[i];
	}
	const _ValT& operator[](int i) const
	{
		return _v[i];
	}
	iterator begin()
	{
		return _v.begin();
	}
	const_iterator begin() const
	{
		return _v.begin();
	}
	iterator end()
	{
		return _v.end();
	}
	const_iterator end() const
	{
		return _v.end();
	}
};

class _CVR_API CVRModelEx
	:public CVRRendable
{
public:
	CVRModel   model;
	Matx44f    mModeli;
	Matx44f    mModel;
public:
	CVRModelEx()
		:mModeli(cvrm::I()),mModel(cvrm::I())
	{}
	CVRModelEx(const CVRModel &_model, const Matx44f &_mModeli = cvrm::I(), const Matx44f &_mModel = cvrm::I());

	virtual void render(const Matx44f &sceneModelView, int flags);
};

class _CVR_API CVRModelArray
	:public CVRArray<CVRModelEx>
{
public:
	CVRModelArray()
	{}
	CVRModelArray(size_t size)
		:CVRArray<CVRModelEx>(size)
	{
	}
	virtual void render(const Matx44f &sceneModelView, int flags);
};

typedef std::shared_ptr<CVRRendable>  CVRRendablePtr;

class _CVR_API CVRRendableArray
	:public CVRArray<CVRRendablePtr>
{
public:
	CVRRendableArray()
	{}
	CVRRendableArray(size_t size)
		:CVRArray<CVRRendablePtr>(size)
	{
	}
	virtual void render(const Matx44f &sceneModelView, int flags);
};


class _CVRender;

class _CVR_API CVRender
{
public:
	class _CVR_API UserDraw
	{
	public:
		virtual void draw() = 0;

		virtual ~UserDraw();
	};

	template<typename _DrawOpT>
	class UserDrawX
		:public UserDraw
	{
		_DrawOpT _op;
	public:
		UserDrawX(const _DrawOpT &op)
			:_op(op)
		{}
		virtual void draw()
		{
			_op();
		}
	};
public:
	std::shared_ptr<_CVRender> impl;
public:
	CVRender();

	CVRender(CVRRendable &rendable);

	CVRender(CVRRendablePtr rendablePtr);

	~CVRender();

	bool empty() const;

	operator bool() const
	{
		return !empty();
	}

	void setBgImage(const cv::Mat &img);

	void clearBgImage()
	{
		this->setBgImage(cv::Mat());
	}

	void setBgColor(float r, float g, float b, float a=1.0f);

	CVRResult exec(CVRMats &mats, Size viewSize, int output=CVRM_IMAGE|CVRM_DEPTH, int flags=CVRM_DEFAULT, UserDraw *userDraw=NULL, cv::Rect outRect=cv::Rect(0,0,0,0));

	//for python call
	CVRResult __exec(CVRMats &mats, Size viewSize, int output = CVRM_IMAGE | CVRM_DEPTH, int flags = CVRM_DEFAULT, cv::Rect outRect = cv::Rect(0, 0, 0, 0))
	{
		return this->exec(mats, viewSize, output, flags, (UserDraw*)NULL, outRect);
	}

	//facilitate user draw with lambda functions
	template<typename _UserDrawOpT>
	CVRResult exec(CVRMats &mats, Size viewSize, _UserDrawOpT &op, int output = CVRM_IMAGE | CVRM_DEPTH, int flags = CVRM_DEFAULT, cv::Rect outRect = cv::Rect(0, 0, 0, 0))
	{
		UserDrawX<_UserDrawOpT> userDraw(op);
		return exec(mats, viewSize, output, flags, &userDraw, outRect);
	}

	//render with a temporary bg-image. The bg-image set by setBgImage will not be changed.
	//CVRResult exec(CVRMats &mats, const cv::Mat &bgImg, int output = CVRM_IMAGE | CVRM_DEPTH, int flags = CVRM_DEFAULT);

	//const CVRModel& model() const;
};

template<typename _DrawOpT>
inline std::shared_ptr<CVRender::UserDraw> newUserDraw(const _DrawOpT &op)
{
	return std::shared_ptr<CVRender::UserDraw>(new CVRender::UserDrawX<_DrawOpT>(op));
}

_CVR_API void drawPoints(const cv::Point3f pts[], int npts, float pointSize=1.0f);


class _CVRTrackBall;

class _CVR_API CVRTrackBall
{
	std::shared_ptr<_CVRTrackBall> impl;
public:
	CVRTrackBall();

	~CVRTrackBall();

	void onMouseDown(int x, int y);

	void onMouseMove(int x, int y, Size viewSize);

	void onMouseWheel(int x, int y, int val);

	void onKeyDown(int key, int flags);

	/* get and apply the transformations
	  rotation and scale are applied separately to model and view transformations
	  if @update==false, the input value of @mModel and @mView will be ignored, otherwise they will be multiplied with the TrackBall transformations
	*/
	void apply(Matx44f &mModel, Matx44f &mView, bool update=true);
};

#include<mutex>
class _CVR_API CVRShowModelBase
{
public:
	class _CVR_API ResultFilter
	{
	public:
		virtual void exec(CVRResult &result) = 0;

		virtual ~ResultFilter();
	};
	template<typename _FilterT>
	class ResultFilterX
		:public ResultFilter
	{
		_FilterT _op;
	public:
		ResultFilterX(const _FilterT &op)
			:_op(op)
		{}
		virtual void exec(CVRResult &result)
		{
			_op(result);
		}
	};
public:
	CVRModel	 model;
	Size         viewSize;
	CVRMats      initMats;
	cv::Mat      bgImg;
	int          renderFlags;
	std::shared_ptr<CVRender::UserDraw> userDraw;
	std::shared_ptr<ResultFilter>  resultFilter;

	CVRender	 render;
	CVRTrackBall trackBall;
	
protected:
	std::mutex   _resultMutex;
	CVRResult    _currentResult;
	bool         _hasResult = false;
public:
	CVRShowModelBase(const CVRModel &_model, Size _viewSize, const CVRMats &_mats, const cv::Mat &_bgImg, int _renderFlags)
		:model(_model),viewSize(_viewSize),initMats(_mats),bgImg(_bgImg),renderFlags(_renderFlags)
	{}

	virtual ~CVRShowModelBase();

	//wait until the background rendering thread becomes idle, which means all rendering messages have been processed
	//call this function before accessing data in the window's event loop, especially for events that may trigger re-rendering, such as mouse-move, left-button-down, left-button-up, etc.
	void waitDone();

	//re-rendering the model. If @waitDone is true, @currentResult will be updated when this function return, otherwise it may have not.
	void update(bool waitDone=false);

	virtual void showModel(const CVRModel &model);

	//called by the bg-thread
	void   setCurrentResult(const CVRResult &r);

	//called in the main-thread
	//return True if has result
	bool   showCurrentResult(bool waitResult=false);

	virtual void present(CVRResult &result) =0;
};

template<typename _FilterOpT>
inline std::shared_ptr<CVRShowModelBase::ResultFilter> newResultFilter(const _FilterOpT &op)
{
	return std::shared_ptr<CVRShowModelBase::ResultFilter>(new CVRShowModelBase::ResultFilterX<_FilterOpT>(op));
}

class _CVRShowModelEvent
{
public:
	enum {
		F_IGNORABLE=0x01
	};
public:
	CVRShowModelBase *winData;
	int code;
	int param1, param2;
	int data;
	int flags=0;
public:
	_CVRShowModelEvent()
	{}
	_CVRShowModelEvent(CVRShowModelBase *_winData, int _code, int _param1, int _param2, int _data, int _flags=0)
		:winData(_winData),code(_code),param1(_param1),param2(_param2), data(_data),flags(_flags)
	{}
};

void _CVR_API _postShowModelEvent(const _CVRShowModelEvent &evt);

//============================================================================
#ifndef CVRENDER_NO_GUI

//show model in CVX windows (based on opencv windows)
#include"CVX/gui.h"

 class CVXShowModel
 	:public CVRShowModelBase
 {
 public:
 	cv::CVWindow *wnd;
 public:
 	CVXShowModel(cv::CVWindow *_wnd, const CVRModel &_model, Size _viewSize, const CVRMats &_mats, const cv::Mat &_bgImg, int _renderFlags)
 		:wnd(_wnd), CVRShowModelBase(_model,_viewSize,_mats,_bgImg,_renderFlags)
 	{}

 	virtual void present(CVRResult &result)
 	{
 		if (wnd && !result.img.empty())
 			wnd->show(result.img);
 	}
 };


typedef std::shared_ptr<CVXShowModel> CVXShowModelPtr;

 inline CVXShowModelPtr mdshow(const std::string &wndName, const CVRModel &model, Size viewSize=Size(800,800), int renderFlags = CVRM_DEFAULT, const cv::Mat bgImg=cv::Mat())
{
	CVXShowModelPtr dptr;
	//return dptr;
	
	cv::CVWindow *wnd = cv::getWindow(wndName, true);
	
	if (wnd)
	{
		//realize the window
		cv::Mat1b img=cv::Mat1b::zeros(viewSize);
		wnd->show(img);

		CVRMats mats(model,viewSize);

		dptr= CVXShowModelPtr(new CVXShowModel(wnd,model, viewSize, mats, bgImg, renderFlags));

		wnd->setEventHandler([dptr](int evt, int param1, int param2, cv::CVEventData data) {
			//dptr->showCurrent();
			_postShowModelEvent(_CVRShowModelEvent(dptr.get(), evt, param1, param2,data.ival,_CVRShowModelEvent::F_IGNORABLE));
			
		}, "cvxMDShowEventHandler");

		dptr->update(true);
		//wait and show the first image
		dptr->showCurrentResult(true);
	}
	return dptr;
}

#endif
