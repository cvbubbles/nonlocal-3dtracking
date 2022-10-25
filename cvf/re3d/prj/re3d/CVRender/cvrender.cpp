
#include"_cvrender.h"



CVRModel::CVRModel()
	:_model(new _CVRModel)
{}
CVRModel::CVRModel(const std::string &file)
	: _model(new _CVRModel)
{
	this->load(file);
}

CVRModel::~CVRModel()
{
}
void CVRModel::clear()
{
	if (_model)
	{
		_model->clear();
	}
}
bool CVRModel::empty() const
{
	return !_model->scene;
}
void CVRModel::load(const std::string &file, int postProLevel)
{
	_model->load(file, postProLevel);
}

void CVRModel::saveAs(const std::string &file, const std::string &fmtID, const std::string &options)
{
	_model->saveAs(file, fmtID, options);
}

Matx44f CVRModel::getUnitize() const
{
	return _model->getModeli();
}
const std::string& CVRModel::getFile() const
{
	return _model->sceneFile;
}
//======================================================

CVRMats::CVRMats(const Matx44f &mInit)
{
	mModeli = mModel = mView = mProjection = mInit;
}

CVRMats::CVRMats(Size viewSize, float fscale, float eyeDist, float zNear, float zFar)
{
	mModeli = mModel = cvrm::I();
	mView = cvrm::lookat(0.f, 0.f, eyeDist, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f);
	mProjection = cvrm::perspective(viewSize.height*fscale, viewSize, zNear, zFar);
}

CVRMats::CVRMats(const CVRModel &model, Size viewSize, float fscale, float eyeDist, float zNear, float zFar)
{
	mModeli = model.getUnitize();
	mModel = cvrm::I();
	mView = cvrm::lookat(0.f, 0.f, eyeDist, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f);
	mProjection = cvrm::perspective(viewSize.height*fscale, viewSize, zNear, zFar);
}

#include<iostream>
using namespace std;

Point3f CVRResult::unproject(float x, float y) const
{
	Point3f p;
	float z;
	if (cv::resampleBL<float>(reinterpret_cast<const Mat1f&>(depth),z, x, y))
	{
		y = img.rows - y;

		Matx44f mModel = mats.mModeli*mats.mModel*mats.mView;
		GLint viewport[4] = { 0,0,img.cols,img.rows };

		//cout << mats.mProjection << endl;

		p = cvrm::unproject(Point3f(x, y, z), mModel, mats.mProjection, viewport);
#if 0
	//	printf("g=(%.2f,%.2f,%.2f)\n", p.x, p.y, p.z);

		Matx44d mModeld = mModel;
		Matx44d mProjd = mats.mProjection;
		double X, Y, Z;
		gluUnProject(x, y, z, mModeld.val, mProjd.val, viewport, &X, &Y, &Z);
		p=Point3f(X, Y, Z);

		gluProject(0.5, -0.5, 0, mModeld.val, mProjd.val, viewport, &X, &Y, &Z);
		printf("prj=(%.2f,%.2f,%.2f)\n", X, Y, Z);

#endif
	}
	else
	{
		CV_Error(ERROR_ACCESS_DENIED, "depth map unavailable or (x,y) out of the range");
	}
	
	return p;
}

cv::Point3f CVRResult::project(float x, float y, float z) const
{
	Matx44f mModel = mats.mModeli*mats.mModel*mats.mView;
	GLint viewport[4] = { 0,0,img.cols,img.rows };

	Point3f p = cvrm::project(Point3f(x, y, z), mModel, mats.mProjection, viewport);
	p.y = img.rows - p.y;
	return p;
}

CVRResult CVRResult::blank(Size viewSize, const CVRMats &_mats)
{
	CVRResult r;
	r.mats = _mats;
	r.img = cv::Mat3b::zeros(viewSize);
	r.depth = cv::Mat1f::zeros(viewSize);
	return r;
}

//==========================================================================================

class _CVRender
{
public:
	CVRModel  _modelx;
	_CVRModel   *_model=nullptr;

	cv::Vec4f    _bgColor = cv::Vec4f(0, 0, 0, 0);
	GLuint		 _bgTexID=0;
public:
	_CVRender()
	{}
	_CVRender(const CVRModel &model)
		:_modelx(model)
	{
		_model =_modelx? _modelx._model.get() : nullptr;

		if (_model)
		{
			cvrCall([this](int) {
				_model->_loadTextures();
			});
		}
	}
	~_CVRender()
	{
		cvrCall([this](int) {
			this->_clearBgImage();
		});
	}
	void _clearBgImage()
	{
		if (_bgTexID > 0)
		{
			glDeleteTextures(1, &_bgTexID);
		}
		_bgTexID = 0;
	}
	void setBgImage(const Mat &img)
	{
		this->_clearBgImage();
		if (!img.empty())
		{
			cv::Mat _bgImg;
			cv::convertBGRChannels(img, _bgImg, 3);
			flip(_bgImg, _bgImg, 0);
			makeSizePower2(_bgImg);
			_bgTexID = loadGLTexture(_bgImg);
			checkGLError();
		}
	}
	void _drawBgImage(Size viewSize)
	{
		if (_bgTexID > 0)
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, _bgTexID);
			int renderWidth = viewSize.width, renderHeight = viewSize.height;

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_LIGHTING);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0, renderWidth, 0, renderHeight);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glBegin(GL_QUADS);
			//glDisable(GL_COLOR_MATERIAL);
			glColor3f(1, 1, 1);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(0, 0);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(renderWidth, 0);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(renderWidth, renderHeight);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(0, renderHeight);
			glEnd();

			glEnable(GL_LIGHTING);
			glEnable(GL_DEPTH_TEST);

			glBindTexture(GL_TEXTURE_2D, 0);
			checkGLError();
		}
	}
	CVRResult exec(const CVRMats &mats, Size viewSize, int output, int flags, CVRender::UserDraw *userDraw)
	{
		if (!_model) //return blank images
			return CVRResult::blank(viewSize, mats);

		theDevice.setSize(viewSize);
		int renderWidth = viewSize.width, renderHeight = viewSize.height;

		glClearColor(_bgColor[0], _bgColor[1], _bgColor[2], _bgColor[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDrawBuffer(GL_BACK);
		glDisable(GL_CULL_FACE);

		if (_bgTexID > 0)
			this->_drawBgImage(viewSize);

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(mats.mProjection.val);
		
		auto mx = mats.mModeli*mats.mModel*mats.mView;
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(mx.val);
		
#if 1
		_model->render(flags);
#else
		glBegin(GL_QUADS);
		glColor3f(1, 1, 1);
		glVertex3f(-0.5, -0.5, 0);
		glColor3f(0, 0, 1);
		glVertex3f(0.5, -0.5, 0);
		glColor3f(0, 1, 0);
		glVertex3f(0.5, 0.5, 0.);
		glColor3f(1, 0, 0);
		glVertex3f(-0.5, 0.5, 0.);
		glEnd();
#endif

		if (userDraw)
			userDraw->draw();

		CVRResult result;
		result.mats = mats;

		glReadBuffer(GL_BACK);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);

		if (output&CVRM_IMAGE)
		{
			cv::Mat dimg(Size(renderWidth, renderHeight), CV_8UC3);
			
			glReadPixels(0, 0, renderWidth, renderHeight, GL_RGB, GL_UNSIGNED_BYTE, dimg.data);

			cvtColor(dimg, dimg, CV_BGR2RGB);
			flip(dimg, dimg, 0);
			result.img = dimg;
		}

		if (output&CVRM_DEPTH)
		{
			cv::Mat depth(Size(renderWidth, renderHeight), CV_32FC1);

			glReadPixels(0, 0, renderWidth, renderHeight, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data);
			flip(depth, depth, 0);
			result.depth = depth;
		}

		checkGLError();

		return result;
	}
#if 0
	CVRResult exec(CVRMats &mats, const cv::Mat &bgImg, int output, int flags)
	{
	//	if (!_device)
	//		return CVRResult();

	//	CVR_LOCK();

		GLuint oldTex = _bgTexID;
		_bgTexID = 0;
		this->setBgImage(bgImg);
		CVRResult result = this->exec(mats, output, flags);
		this->_clearBgImage();
		_bgTexID = oldTex;
		return result;
	}
#endif
};

CVRender::UserDraw::~UserDraw()
{}

CVRender::CVRender()
	:impl(new _CVRender())
{}

CVRender::CVRender(const CVRModel &model)
	:impl(new _CVRender(model))
{}
CVRender::~CVRender()
{
}
bool CVRender::empty() const
{
	return !impl->_model;
}
void CVRender::setBgImage(const cv::Mat &img)
{
	cvrCall([&](int) {
		impl->setBgImage(img);
	});
}
void CVRender::setBgColor(float r, float g, float b, float a)
{
	impl->_bgColor = Vec4f(r, g, b, a);
}
CVRResult CVRender::exec(CVRMats &mats, Size viewSize, int output, int flags, CVRender::UserDraw *userDraw)
{
	CVRResult r;
	cvrCall([&](int) {
		r=impl->exec(mats, viewSize, output, flags, userDraw);
	});
	return r;
}
//CVRResult CVRender::exec(CVRMats &mats, const cv::Mat &bgImg, int output, int flags)
//{
//	CVRResult r;
//	cvrCall([&](int) {
//		r = impl->exec(mats, bgImg, output, flags);
//	});
//	return r;
//}
const CVRModel& CVRender::model() const
{
	return impl->_modelx;
}

_CVR_API void drawPoints2(const cv::Point3f pts[], int npts, float pointSize)
{
	glPointSize(pointSize);
	glColor3f(1, 0, 0);
	//glBegin(GL_POINTS);

	glMatrixMode(GL_MODELVIEW);
	double M[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, M);
	for (int i = 0; i < npts; ++i)
	{	//glVertex3f(pts[i].x, pts[i].y, pts[i].z);
		glLoadMatrixd(M);
		glTranslatef(pts[i].x, pts[i].y, pts[i].z);
		glutSolidSphere(0.3, 30, 30);
	}
	//glEnd();
}

_CVR_API void drawPoints(const cv::Point3f pts[], int npts, float pointSize)
{
	glPointSize(pointSize);
	glColor3f(1, 0, 0);
	glBegin(GL_POINTS);

	for (int i = 0; i < npts; ++i)
	{	
		glVertex3f(pts[i].x, pts[i].y, pts[i].z);
	}
	glEnd();
}

void CVRShowModelBase::waitDone()
{
	cvrWaitFinish();
}

void CVRShowModelBase::update(bool waitDone)
{
	//simulate user interactions for  triggering redraw
	_postShowModelEvent(_CVRShowModelEvent(this, cv::EVENT_LBUTTONDOWN, 0, 0, 0));
	_postShowModelEvent(_CVRShowModelEvent(this, cv::EVENT_MOUSEMOVE, 0, 0, cv::EVENT_FLAG_LBUTTON));

	if (waitDone)
		cvrWaitFinish();
}

void CVRShowModelBase::showModel(const CVRModel &model)
{
	this->waitDone();

	this->model = model;
	render = CVRender(model);
	initMats = CVRMats(model, viewSize);
	this->update(false);
}

CVRShowModelBase::~CVRShowModelBase()
{}


void proShowModelEvents(const _CVRShowModelEvent &evt, int nLeft)
{
	if (nLeft > 0 && (evt.flags&_CVRShowModelEvent::F_IGNORABLE))
		return;
	else if(evt.winData)
	{
		auto *wd = evt.winData;

		if (!wd->model)
		{
			wd->currentResult = CVRResult::blank(wd->viewSize, wd->initMats);
			return;
		}

		if (!wd->render)
		{
			wd->render = CVRender(wd->model);
			if (!wd->bgImg.empty())
				wd->render.setBgImage(wd->bgImg);
		}

		time_t beg = clock();

		auto &tb(evt.winData->trackBall);

		if (evt.code == cv::EVENT_MOUSEMOVE)
			tb.onMouseMove(evt.param1, evt.param2, theDevice.size());
		else if (evt.code == cv::EVENT_MOUSEWHEEL)
			tb.onMouseWheel(evt.param1, evt.param2, evt.data);

		CVRMats mats = evt.winData->initMats;
		tb.apply(mats.mModel, mats.mView, true);

		{
			wd->currentResult = wd->render.exec(mats, wd->viewSize, CVRM_IMAGE | CVRM_DEPTH, wd->renderFlags, wd->userDraw.get());
		}
		postShowImage(wd);
	}
}

void _CVR_API _postShowModelEvent(const _CVRShowModelEvent &evt)
{
	if (evt.code==cv::EVENT_LBUTTONDOWN)
	{
		evt.winData->trackBall.onMouseDown(evt.param1, evt.param2);
	}
	else if(
		evt.code==cv::EVENT_MOUSEMOVE && (evt.data&cv::EVENT_FLAG_LBUTTON) 
		|| evt.code==cv::EVENT_MOUSEWHEEL
		)
	{
		cvrPost([evt](int nLeft) {
			proShowModelEvents(evt, nLeft);
		}, false);
	}
	else if (evt.code == cv::EVENT_KEYBOARD)
	{
		int key = evt.param1;
		auto wd = evt.winData;
		//use upper case to trigger key control
		switch (key)
		{
		case 'L': //toggle lighting
			wd->renderFlags ^= CVRM_ENABLE_LIGHTING;
			break;
		case 'M': //toggle material
			wd->renderFlags ^= CVRM_ENABLE_MATERIAL;
			break;
		case 'T': //toggle texture
			wd->renderFlags ^= CVRM_ENABLE_TEXTURE;
			break;
		case 'R': // no lighting for object with texture
			wd->renderFlags ^= CVRM_TEXTURE_NOLIGHTING;
			break;
		default:
			key = 0;
		}
		if (key != 0)
			wd->update(false);
	}
}


