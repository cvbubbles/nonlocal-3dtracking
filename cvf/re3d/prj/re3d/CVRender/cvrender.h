#pragma once

#ifdef CVRENDER_EXPORTS
#define _CVR_API __declspec(dllexport)
#else
#define _CVR_API __declspec(dllimport)
#endif

#include"opencv2/core/core.hpp"
#include<memory>

using cv::Matx44f;
using cv::Size;

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

	static Matx44f rotate(float angle, const cv::Vec3f &axis);

	static Matx44f perspective(float f, Size windowSize, float nearP, float farP);

	//conver intrinsic parameters to projection matrix
	static Matx44f perspective(float fx, float fy, float cx, float cy, Size windowSize, float nearP, float farP);

	static Matx44f fromK(const cv::Mat &K, Size windowSize, float nearP, float farP);

	//convert rotation and translation vectors to model-view matrix
	static Matx44f fromRT(const cv::Mat &rvec, const cv::Mat &tvec);

	static Matx44f lookat(float eyex, float eyey, float eyez, float centerx, float centery, float centerz, float upx, float upy, float upz);

	static cv::Point3f unproject(const cv::Point3f &winPt, const Matx44f &mModelview, const Matx44f &mProjection, const int viewport[4]);

	static cv::Point3f project(const cv::Point3f &objPt, const Matx44f &mModelview, const Matx44f &mProjection, const int viewport[4]);

	//rotation M that transforms s to t : t=s*M;
	static Matx44f  rotate(const cv::Vec3f &s, const cv::Vec3f &t);

	static void  sampleSphere(std::vector<cv::Vec3f> &vecs, int n);
};

// y=x*M;
_CVR_API cv::Vec3f operator*(const cv::Vec3f &x, const Matx44f &M);

_CVR_API cv::Point3f operator*(const cv::Point3f &x, const Matx44f &M);

struct _CVRModel;

class _CVR_API CVRModel
{
	friend class _CVRender;
	std::shared_ptr<_CVRModel> _model;
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
	void load(const std::string &file, int postProLevel=3);

	/* save the model to a file, any file format that supported by Assimp can be used, such as .obj, .3ds, .ply, .stl, .dae ,...
	 @fmtID : a string to specify the file format, the file extension will be used if is empty.
	 @options : additional options, use "-std" to rename the texture image files in a standard way.
	*/
	void saveAs(const std::string &file, const std::string &fmtID = "", const std::string &options="-std");

	//get a transformation that transform the model to a standard position and size (centered at the origin and scale to fit in a unit cube)
	Matx44f getUnitize() const;

	const std::string& getFile() const;
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
	CVRMats(Size viewSize, float fscale = 1.5f, float eyeDist = 2.0f, float zNear = 0.1, float zFar = 100);

	//set the matrixs to show a model in a standard way (used in mdshow(...) )
	CVRMats(const CVRModel &model, Size viewSize, float fscale = 1.5f, float eyeDist = 2.0f, float zNear = 0.1, float zFar = 100);

	Matx44f modelView() const
	{
		return mModeli*mModel*mView;
	}
};

class _CVR_API CVRResult
{
public:
	cv::Mat  img;
	cv::Mat  depth;
	CVRMats    mats;
public:
	cv::Point3f unproject(float x, float y) const;

	cv::Point3f unproject(const cv::Point2f &pt) const
	{
		return unproject(pt.x, pt.y);
	}

	cv::Point3f project(float x, float y, float z) const;

	cv::Point3f project(const cv::Point3f &pt) const
	{
		return project(pt.x, pt.y, pt.z);
	}

	static CVRResult blank(Size viewSize, const CVRMats &_mats);
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
	CVRM_DEPTH=0x02
//	CVRM_ALPHA=0x04
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

	CVRender(const CVRModel &model);

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

	CVRResult exec(CVRMats &mats, Size viewSize, int output=CVRM_IMAGE|CVRM_DEPTH, int flags=CVRM_DEFAULT, UserDraw *userDraw=NULL);

	//facilitate user draw with lambda functions
	template<typename _UserDrawOpT>
	CVRResult exec(CVRMats &mats, Size viewSize, _UserDrawOpT &op, int output = CVRM_IMAGE | CVRM_DEPTH, int flags = CVRM_DEFAULT)
	{
		UserDrawX<_UserDrawOpT> userDraw(op);
		return exec(mats, viewSize, output, flags, &userDraw);
	}

	//render with a temporary bg-image. The bg-image set by setBgImage will not be changed.
	//CVRResult exec(CVRMats &mats, const cv::Mat &bgImg, int output = CVRM_IMAGE | CVRM_DEPTH, int flags = CVRM_DEFAULT);

	const CVRModel& model() const;
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

	/* get and apply the transformations
	  rotation and scale are applied separately to model and view transformations
	  if @update==false, the input value of @mModel and @mView will be ignored, otherwise they will be multiplied with the TrackBall transformations
	*/
	void apply(Matx44f &mModel, Matx44f &mView, bool update=true);
};


class _CVR_API CVRShowModelBase
{
public:
	CVRModel	 model;
	Size         viewSize;
	CVRMats      initMats;
	cv::Mat      bgImg;
	int          renderFlags;
	std::shared_ptr<CVRender::UserDraw> userDraw;

	CVRender	 render;
	CVRTrackBall trackBall;

	CVRResult    currentResult;
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

	virtual void present(const CVRResult &result) =0;
};

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

	virtual void present(const CVRResult &result)
	{
		if (wnd && !result.img.empty())
			wnd->show(result.img);
	}
};

typedef std::shared_ptr<CVXShowModel> CVXShowModelPtr;

inline CVXShowModelPtr mdshow(const std::string &wndName, const CVRModel &model, Size viewSize=Size(800,800), int renderFlags = CVRM_DEFAULT, const cv::Mat &bgImg=cv::Mat())
{
	cv::CVWindow *wnd = cv::getWindow(wndName, true);
	CVXShowModelPtr dptr;
	if (wnd)
	{
		//realize the window
		cv::Mat1b img=cv::Mat1b::zeros(viewSize);
		wnd->show(img);

		CVRMats mats(model,viewSize);

		dptr= CVXShowModelPtr(new CVXShowModel(wnd,model, viewSize, mats, bgImg, renderFlags));

		wnd->setEventHandler([dptr](int evt, int param1, int param2, cv::CVEventData data) {
			
			_postShowModelEvent(_CVRShowModelEvent(dptr.get(), evt, param1, param2,data.ival,_CVRShowModelEvent::F_IGNORABLE));
			
		}, "cvxMDShowEventHandler");

		dptr->update(false);
	}
	return dptr;
}

