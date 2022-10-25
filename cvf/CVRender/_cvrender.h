#pragma once

#define USE_ASSIMP
//#define USE_VCGLIB

#ifdef USE_ASSIMP
#include "assimp/scene.h"
#include "assimp/cimport.h"
#include "assimp/postprocess.h"
#include"assimp/Exporter.hpp"
#elif defined(USE_VCGLIB)
#define  MESHLAB_SCALAR float
#include"common/ml_mesh_type.h"
#endif

#define USE_GLUT

#ifdef _WIN32

#if defined(USE_GLUT)
#define FREEGLUT_STATIC
#include"GL/freeglut.h"
#else
#include"GL/glew.h"
#include"GLFW/glfw3.h"
#endif

#else
#   include <GL/gl.h>
#   include <GL/glu.h>
#endif

#include"opencv2/highgui.hpp"
#include"opencv2/imgproc.hpp"
using namespace cv;

#include"BFC/err.h"
#include"BFC/stdf.h"
#include"BFC/portable.h"

#include<map>
#include<memory>
#include<time.h>

#include"cvrender.h"

/*
* GLUT API macro definitions -- the display mode definitions
*/
#define  CVRW_RGB                           0x0000
#define  CVRW_RGBA                          0x0000
#define  CVRW_INDEX                         0x0001
#define  CVRW_SINGLE                        0x0000
#define  CVRW_DOUBLE                        0x0002
#define  CVRW_ACCUM                         0x0004
#define  CVRW_ALPHA                         0x0008
#define  CVRW_DEPTH                         0x0010
#define  CVRW_STENCIL                       0x0020
#define  CVRW_MULTISAMPLE                   0x0080
#define  CVRW_STEREO                        0x0100
#define  CVRW_LUMINANCE                     0x0200

#define CVRW_GLUT_MASK 0xFFFF

#define CVRW_VISIBLE    0x010000

#define CVRW_DEFAULT (CVRW_RGBA|CVRW_DOUBLE|CVRW_DEPTH/*|CVRW_VISIBLE*/)


class _CVRDevice;

//a hidden GLUT window to represent an OpenGL context
class CVRDevice
{
	std::shared_ptr<_CVRDevice> _impl;
public:
	CVRDevice();

	CVRDevice(Size size, int flags = CVRW_DEFAULT, const std::string &wndName = "");

	~CVRDevice();

	void create(Size size, int flags = CVRW_DEFAULT, const std::string &wndName = "");

	int width() const {
		return size().width;
	}
	int height() const {
		return size().height;
	}
	Size size() const;

	bool empty() const;

	bool operator!() const
	{
		return empty();
	}

	//make the GL context the current
	void setCurrent();

	void setSize(int width, int height);

	void setSize(const Size &size)
	{
		this->setSize(size.width, size.height);
	}
	void postRedisplay();
};



class GLEvent
{
public:
	virtual void exec(int nLeft) = 0;

	virtual void release()
	{
		delete this;
	}

	virtual ~GLEvent()
	{}
};

template<typename _OpT>
class GLEventX
	:public GLEvent
{
	_OpT _op;
public:
	GLEventX()
	{}
	GLEventX(const _OpT &op)
		:_op(op)
	{}
	virtual void exec(int nLeft)
	{
		_op(nLeft);
	}
};

extern CVRDevice  theDevice;

void cvrWaitFinish();

void cvrPostEvent(GLEvent *evt, bool wait);

template<typename _OpT>
inline void cvrPost(const _OpT &op, bool wait)
{
	cvrPostEvent(new GLEventX<_OpT>(op), wait);
}
template<typename _OpT>
inline void cvrCall(const _OpT &op)
{
	cvrPost(op, true);
}


struct TexImage
{
	std::string file;
	cv::Mat     img;
};

struct _Texture
{
	GLuint texID;
};

typedef std::map<std::string, _Texture> TexMap;

void makeSizePower2(Mat &img);

GLuint loadGLTexture(const Mat3b &img);

struct _CVRModel
{
public:
	std::string    sceneFile;
	//Matx44f        _sceneTransform=cvrm::I();

#if defined(USE_ASSIMP)
	aiMatrix4x4    _sceneTransformInFile;
	aiScene *scene = nullptr;
	aiVector3D scene_min, scene_max, scene_center;
#endif

#if defined(USE_VCGLIB)
	Matrix44m  _sceneTransformInFile;
	std::shared_ptr<CMeshO> scene;
	Point3f scene_min, scene_max, scene_center;
#endif

	std::vector<TexImage> vTex;

	TexMap       _texMap;
	GLuint		_sceneList = 0;
	int			_sceneListRenderFlags = -1;

	std::vector<Vec3f> _vertices;

public:
	~_CVRModel()
	{
		this->clear();
	}
	void clear()
	{
#if defined(USE_ASSIMP)
		if (scene)
		{
			aiReleaseImport(scene);
			scene = nullptr;
			sceneFile = "";
		}
#endif

		vTex.clear();
		if (!_texMap.empty() || _sceneList!=0)
		{
			cvrCall([this](int) {
				for (auto &t : _texMap)
				{
					if (t.second.texID > 0)
						glDeleteTextures(1, &t.second.texID);
				}
				_texMap.clear();

				if (_sceneList != 0)
				{
					glDeleteLists(_sceneList, 1);
					_sceneList = 0;
					_sceneListRenderFlags = -1;
				}
			});
			cvrWaitFinish();
		}
	}

	void saveAs(const std::string &file, std::string fmtID, const std::string & options);
	
	//void _loadExtFile(const std::string &file, ff::ArgSet &args);

	void load(const std::string &file, int postProLevel, const std::string &options);

	void _updateSceneInfo();
	
	void _loadTextures();

	bool _texNotLoaded()
	{
		return !vTex.empty() && _texMap.empty();
	}

	void render(int flags);

	const std::vector<Vec3f>& getVertices();

	void  getMesh(CVRMesh &mesh, int flags);

	Matx44f calcStdPose();

	void  setSceneTransformation(const Matx44f &trans, bool updateSceneInfo=true);

	/*Matx44f getSceneTransformation() const
	{
		return _sceneTransform;
	}*/
};

void postShowImage(CVRShowModelBase *winData);


inline void _checkGLErrors(const char *funcName, const std::string tag="")
{
	GLenum error;

	while ((error = glGetError()) != GL_NO_ERROR)
	{
		if (!tag.empty())
			printf("[%s]:", tag.c_str());
		printf("OpenGL Error in %s: %s\n", funcName, gluErrorString(error));
	}
}

#define checkGLError() _checkGLErrors(__FUNCTION__);
#define checkGLErrorEx(tag) _checkGLErrors(__FUNCTION__,tag);

