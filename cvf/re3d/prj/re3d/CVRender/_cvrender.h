#pragma once

#include "assimp/scene.h"
#include "assimp/cimport.h"
#include "assimp/postprocess.h"
#include"assimp/Exporter.hpp"

#include"GL/freeglut.h"

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

#define CVRW_DEFAULT (CVRW_RGBA|CVRW_DOUBLE|CVRW_DEPTH)





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
	aiScene *scene = nullptr;
	aiVector3D scene_min, scene_max, scene_center;

	std::vector<TexImage> vTex;

	TexMap       _texMap;
	GLuint		_sceneList = 0;
	int			_sceneListRenderFlags = -1;
public:
	~_CVRModel()
	{
		this->clear();
	}
	void clear()
	{
		if (scene)
		{
			aiReleaseImport(scene);
			scene = nullptr;
			sceneFile = "";
		}
		vTex.clear();
		if (!_texMap.empty())
		{
			cvrCall([this](int) {
				for (auto &t : _texMap)
				{
					if (t.second.texID > 0)
						glDeleteTextures(1, &t.second.texID);
				}
			});
			_texMap.clear();
		}
		if (_sceneList != 0)
		{
			glDeleteLists(_sceneList, 1);
			_sceneList = 0;
			_sceneListRenderFlags = -1;
		}
	}

	void saveAs(const std::string &file, std::string fmtID, const std::string & options);
	
	void load(const std::string &file, int postProLevel);
	

	void _loadTextures();
	

	void render(int flags);

	Matx44f getModeli()
	{
		if (!scene)
			return cvrm::I();
		else
		{
			float tmp = scene_max.x - scene_min.x;
			tmp = __max(scene_max.y - scene_min.y, tmp);
			tmp = __max(scene_max.z - scene_min.z, tmp);
			tmp = 1.f / tmp;

			return cvrm::translate(-scene_center.x, -scene_center.y, -scene_center.z) * cvrm::scale(tmp, tmp, tmp);
		}
	}
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

