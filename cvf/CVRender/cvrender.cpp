
#include"_cvrender.h"
#include<iostream>
#include<thread>

CVRRendable::~CVRRendable()
{}
void CVRRendable::setVisible(bool visible)
{
	_visible = visible;
}

void CVRModel::render(const Matx44f &sceneModelView, int flags)
{
	if (_model)
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(sceneModelView.val);

		if (this->isVisible())
		{
			if (_model->_texNotLoaded())
				_model->_loadTextures();

			_model->render(flags);
		}
	}
}

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
void CVRModel::load(const std::string &file, int postProLevel, const std::string &options)
{
	//try {
	_model->load(file, postProLevel, options);
	/*}
	catch (const std::exception &ec)
	{
		printf("%s\n", ec.what());
	}*/
}

void CVRModel::saveAs(const std::string &file, const std::string &fmtID, const std::string &options)
{
	_model->saveAs(file, fmtID, options);
}

Matx44f CVRModel::getUnitize(const cv::Vec3f &center, const cv::Vec3f &bbMin, const cv::Vec3f &bbMax)
{
	float tmp = 0;
	for (int i = 0; i < 3; ++i)
		tmp = __max(bbMax[i] - bbMin[i], tmp);
	tmp = 2.f / tmp;

	return cvrm::translate(-center[0], -center[1], -center[2]) * cvrm::scale(tmp, tmp, tmp);
}

Matx44f CVRModel::getUnitize() const
{
	if (!_model->scene)
		return cvrm::I();

	Vec3f bbMin, bbMax;
	this->getBoundingBox(bbMin, bbMax);
	return getUnitize(this->getCenter(), bbMin, bbMax);
}

cv::Vec3f CVRModel::getCenter() const
{
	auto &c = _model->scene_center;
	return cv::Vec3f(c.x, c.y, c.z);
}
const std::vector<cv::Vec3f>& CVRModel::getVertices() const
{
	return _model->getVertices();
}
void CVRModel::getMesh(CVRMesh &mesh, int flags)
{
	_model->getMesh(mesh, flags);
}
void    CVRModel::getBoundingBox(cv::Vec3f &cMin, cv::Vec3f &cMax) const
{
	static_assert(sizeof(cMin) == sizeof(_model->scene_min),"incompatible type for memcpy");
	memcpy(&cMin, &_model->scene_min, sizeof(cMin));
	memcpy(&cMax, &_model->scene_max, sizeof(cMax));
}
cv::Vec3f CVRModel::getSizeBB() const
{
	cv::Vec3f cMin, cMax;
	this->getBoundingBox(cMin, cMax);
	return cMax - cMin;
}
const std::string& CVRModel::getFile() const
{
	return _model->sceneFile;
}
Matx44f CVRModel::estimatePose0() const
{
	return _model->calcStdPose();
}
void CVRModel::transform(const Matx44f &trans)
{
	_model->setSceneTransformation(trans);
}
//Matx44f CVRModel::getPose0() const
//{
//	return _model->_sceneTransform;
//}

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
#if 0
	auto center = model.getCenter();
	Vec3f bbMin, bbMax;
	model.getBoundingBox(bbMin, bbMax);

	float scale = 0;
	for (int i = 0; i < 3; ++i)
		scale = __max(bbMax[i] - bbMin[i], scale);
	scale = 2.f / scale;
	eyeDist /= scale;

	Vec3f bbSize = bbMax - bbMin;
	float objSize = sqrt(bbSize.dot(bbSize));
	
	mModeli = cvrm::translate(-center[0], -center[1], -center[2]);
	mModel = cvrm::I();
	mView = cvrm::lookat(0.f, 0.f, eyeDist, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f);
	mProjection = cvrm::perspective(viewSize.height*fscale, viewSize, __max(eyeDist-objSize,0.1f), eyeDist+objSize);
#else
	this->setUtilizedModelView(model, eyeDist);
	mProjection = cvrm::perspective(viewSize.height*fscale, viewSize, zNear, zFar);
#endif
}

void CVRMats::setUtilizedModelView(const CVRModel &model,float eyeDist)
{
	mModeli = model.getUnitize();
	mModel = cvrm::I();
	mView = cvrm::lookat(0.f, 0.f, eyeDist, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f);
}

static const float BASE_SIZE_SCALE = 1.2f;

void CVRMats::setInImage(const CVRModel &model, Size viewSize, const Matx33f &K, float sizeScale, float zNear, float zFar)
{
	this->setInROI(model,viewSize,Rect(0,0,viewSize.width,viewSize.height),K,sizeScale,zNear,zFar);
}
 
void CVRMats::setInROI(const CVRModel &model, Size viewSize, Rect roi, const Matx33f &K, float sizeScale, float zNear, float zFar)
{
	sizeScale *= BASE_SIZE_SCALE;

	float fi = (K(0, 0) + K(1, 1))*0.5f;
	float fscale = fi / float(viewSize.height);
	
	auto center = model.getCenter();
	Vec3f bbMin, bbMax;
	model.getBoundingBox(bbMin, bbMax);

	Vec3f bbSize = bbMax - bbMin;
	float objSize = sqrt(bbSize.dot(bbSize));

	float f = fscale*2.0f;
	sizeScale *= float(roi.height) / viewSize.height;

	float eyeDist = f*objSize / 2.0f / sizeScale;  //

	float yscale = 2.0f / viewSize.height;
	Point2f roiCenter(roi.x + roi.width / 2 - viewSize.width / 2, -(roi.y + roi.height / 2 - viewSize.height / 2));
	roiCenter *= yscale * eyeDist / f;

	mModeli = cvrm::translate(-center[0], -center[1], -center[2]);
	mModel = cvrm::I();
	mView = cvrm::translate(roiCenter.x, roiCenter.y, 0)*cvrm::lookat(0.f, 0.f, eyeDist, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f);

	if(zNear<=0.f)
		zNear=__max(0.1f, eyeDist-objSize);
	if(zFar<=0.f)
		zFar=eyeDist+objSize;
	mProjection=cvrm::fromK(K,viewSize,zNear,zFar);
}

void CVRResult::getDepthRange(float &minDepth, float &maxDepth) const
{
	Mat1f depthf = this->depth;
	minDepth = 1.0f;
	maxDepth = 0.0f;
	for_each_1(DWHN1(depthf), [&minDepth, &maxDepth](float d) mutable {
		if (d < minDepth)
			minDepth = d;
		else if (d<0.999999 && d>maxDepth)
			maxDepth = d;
	});
}
float CVRResult::getDepthRange() const
{
	float minDepth, maxDepth;
	getDepthRange(minDepth, maxDepth);
	return maxDepth - minDepth;
}
Mat1f CVRResult::getNormalizedDepth() const
{
	float minDepth, maxDepth;
	this->getDepthRange(minDepth, maxDepth);
	
	Mat1f dn = depth.clone();
	float scale = 1.0f / (maxDepth - minDepth);
	for_each_1(DWHN1(dn), [minDepth, maxDepth, scale](float &f) {
		f = f < maxDepth ? (f - minDepth)*scale : 1.0f;
	});
	return dn;
}
bool  CVRResult::getDepth(float x, float y, float &d) const
{
	bool r = false;
	if (uint(int(x + 0.5)) < uint(depth.cols) && uint(int(y + 0.5)) < uint(depth.rows))
	{
		resampleBL(reinterpret_cast<const cv::Mat1f&>(depth), d, x, y);
		r = true;
	}
	return r;
}
Mat1b CVRResult::getMaskFromDepth(float eps)
{
	Mat1b mask = Mat1b::zeros(depth.size());
	for_each_2(DWHN1(depth), DN1(mask), [eps](float d, uchar& m) {
		m = fabs(1.0f - d) < eps ? 0 : 255;
	});
	return mask;
}

CVRResult CVRResult::blank(Size viewSize, const CVRMats &_mats)
{
	CVRResult r;
	r.mats = _mats;
	r.img = cv::Mat3b::zeros(viewSize);
	r.depth = cv::Mat1f::zeros(viewSize);
	return r;
}

CVRProjector::CVRProjector()
{
}

CVRProjector::CVRProjector(const CVRResult &rr, cv::Size viewSize)
{
	auto &mats(rr.mats);
	_mModelView = mats.mModeli*mats.mModel*mats.mView;
	_mProjection = mats.mProjection;

	//viewSize must be specified if outRect is not the full image
	CV_Assert(rr.outRect.x==0&&rr.outRect.y==0 || viewSize.width>0 && viewSize.height>0);

	Size sz = viewSize.width>0&&viewSize.height>0? viewSize : !rr.img.empty() ? rr.img.size() : rr.depth.size();
	CV_Assert(sz.width > 0 && sz.height > 0);
	//CV_Assert(rr.outRect.size() == sz);

	_viewport = cv::Vec4i(0, 0, sz.width, sz.height);
	_depth = rr.depth;
	_depthOffset = Point2f((float)rr.outRect.x, (float)rr.outRect.y);
}
CVRProjector::CVRProjector(const CVRMats &mats, cv::Size viewSize)
{
	_mModelView = mats.mModeli*mats.mModel*mats.mView;
	_mProjection = mats.mProjection;
	_viewport = cv::Vec4i(0, 0, viewSize.width, viewSize.height);
	_depthOffset = Point2f(0, 0);
}
CVRProjector::CVRProjector(const cv::Matx44f &mModelView, const cv::Matx44f &mProjection, cv::Size viewSize)
	:_mModelView(mModelView), _mProjection(mProjection), _viewport(0, 0, viewSize.width, viewSize.height)
{
	_depthOffset = Point2f(0, 0);
}
CVRProjector::CVRProjector(const cv::Matx44f &mModelView, const cv::Matx33f &cameraK, cv::Size viewSize, float nearP, float farP)
	: _mModelView(mModelView), _mProjection(cvrm::fromK(cameraK, viewSize, nearP, farP)), _viewport(0, 0, viewSize.width, viewSize.height)
{
	_depthOffset = Point2f(0, 0);
}

cv::Point3f CVRProjector::unproject(float x, float y) const
{
	float z;
	if (!cv::resampleBL<float>(reinterpret_cast<const Mat1f&>(_depth), z, x - _depthOffset.x, y - _depthOffset.y))
		//CV_Error(0, "depth map unavailable or (x,y) out of the range");
		z = 1.0f;

	return unproject(x, y, z);
}

CVRModelEx::CVRModelEx(const CVRModel &_model, const Matx44f &_mModeli, const Matx44f &_mModel)
	:model(_model),mModeli(_mModeli),mModel(_mModel)
{}

void CVRModelEx::render(const Matx44f &sceneModelView, int flags)
{
	Matx44f mx = mModeli*mModel*sceneModelView;
	model.render(mx, flags);
}

void CVRModelArray::render(const Matx44f &sceneModelView, int flags)
{
	for (size_t i = 0; i < _v.size(); ++i)
		_v[i].render(sceneModelView, flags);
}

void CVRRendableArray::render(const Matx44f &sceneModelView, int flags)
{
	for (size_t i = 0; i < _v.size(); ++i)
		_v[i]->render(sceneModelView, flags);
}

//==========================================================================================


class _CVRender
{
public:
	//CVRModelEx  _modelx;
	CVRRendablePtr _rendablePtr;
	CVRRendable  *_rendable = nullptr;

	cv::Vec4f    _bgColor = cv::Vec4f(0, 0, 0, 0);
	GLuint		 _bgTexID=0;
public:
	_CVRender()
	{}
	_CVRender(CVRRendable &rendable)
		:_rendable(&rendable)
	{
	}
	_CVRender(const CVRRendablePtr &rendablePtr)
		:_rendablePtr(rendablePtr),_rendable(rendablePtr.get())
	{
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
	CVRResult exec(const CVRMats &mats, Size viewSize, int output, int flags, CVRender::UserDraw *userDraw, Rect outRect)
	{
		//if (!_rendable) //return blank images
		//	return CVRResult::blank(viewSize, mats);

		theDevice.setSize(viewSize);
		int renderWidth = viewSize.width, renderHeight = viewSize.height;

		/*GLuint fbo;
		glGenFramebuffersEXT(1, &fbo);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

		GLuint dbo;
		glGenRenderbuffersEXT(1, &dbo);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, dbo);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT32F, renderWidth, renderHeight);*/
		

		GLint x;
		glGetIntegerv(GL_DEPTH_BITS, &x);

		glClearColor(_bgColor[0], _bgColor[1], _bgColor[2], _bgColor[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDrawBuffer(GL_BACK);
		glDisable(GL_CULL_FACE);

		if (_bgTexID > 0)
			this->_drawBgImage(viewSize);

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(mats.mProjection.val);
		
		auto sceneModelView = mats.mModeli*mats.mModel*mats.mView;
		

#if 1
		//time_t beg = clock();
		if(_rendable)
			_rendable->render(sceneModelView, flags);
		//printf("cvrender: time=%d\n", int(clock() - beg));
#else
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(sceneModelView.val);

		glBegin(GL_QUADS);
		glColor3f(1, 1, 1);
		float z = 0.;
		glVertex3f(-0.5, -0.5, z);
		//glColor3f(0, 0, 1);
		glVertex3f(0.5, -0.5, z);
		//glColor3f(0, 1, 0);
		glVertex3f(0.5, 0.5, z);
		//glColor3f(1, 0, 0);
		glVertex3f(-0.5, 0.5, z);
		glEnd();
#endif



		if (userDraw)
			userDraw->draw();

		CVRResult result;
		result.mats = mats;

		if (outRect.width == 0 && outRect.height == 0)
			outRect = Rect(0, 0, renderWidth, renderHeight);
		else
			outRect = rectOverlapped(outRect, Rect(0, 0, renderWidth, renderHeight));
		result.outRect = outRect;

		if (outRect.width <= 0 || outRect.height <= 0)
			return result;

		glReadBuffer(GL_BACK);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);

		//time_t beg = clock();
		outRect.y = renderHeight - (outRect.y + outRect.height);
		if (output&CVRM_IMAGE)
		{
			cv::Mat dimg(Size(outRect.width, outRect.height), CV_8UC3);
			
			glReadPixels(outRect.x, outRect.y, outRect.width, outRect.height, GL_RGB, GL_UNSIGNED_BYTE, dimg.data);

			cvtColor(dimg, dimg, CV_BGR2RGB);
			//if(output & CVRM_FLIP)
				flip(dimg, dimg, 0);
			result.img = dimg;
		}

		if (output&CVRM_DEPTH)
		{
			cv::Mat depth(Size(outRect.width, outRect.height), CV_32FC1);

			glReadPixels(outRect.x, outRect.y, outRect.width, outRect.height, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data);
			//if (output & CVRM_FLIP)
				flip(depth, depth, 0);

			result.depth = depth;
		}
		//printf("cvrender read: time=%d\n", int(clock() - beg));

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

CVRender::CVRender(CVRRendable &rendable)
	:impl(new _CVRender(rendable))
{}
CVRender::CVRender(CVRRendablePtr rendablePtr)
	: impl(new _CVRender(rendablePtr))
{}
CVRender::~CVRender()
{
}
bool CVRender::empty() const
{
	return !impl->_rendable;
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
CVRResult CVRender::exec(CVRMats &mats, Size viewSize, int output, int flags, CVRender::UserDraw *userDraw, Rect outRect)
{
	CVRResult r;
	cvrCall([&](int) {
		r=impl->exec(mats, viewSize, output, flags, userDraw, outRect);
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
//const CVRModel& CVRender::model() const
//{
//	return impl->_modelx;
//}
//
//_CVR_API void drawPoints2(const cv::Point3f pts[], int npts, float pointSize)
//{
//	glPointSize(pointSize);
//	glColor3f(1, 0, 0);
//	//glBegin(GL_POINTS);
//
//	glMatrixMode(GL_MODELVIEW);
//	double M[16];
//	glGetDoublev(GL_MODELVIEW_MATRIX, M);
//	for (int i = 0; i < npts; ++i)
//	{	//glVertex3f(pts[i].x, pts[i].y, pts[i].z);
//		glLoadMatrixd(M);
//		glTranslatef(pts[i].x, pts[i].y, pts[i].z);
//		glutSolidSphere(0.3, 30, 30);
//	}
//	//glEnd();
//}
//
//_CVR_API void drawPoints(const cv::Point3f pts[], int npts, float pointSize)
//{
//	glPointSize(pointSize);
//	glColor3f(1, 0, 0);
//	glBegin(GL_POINTS);
//
//	for (int i = 0; i < npts; ++i)
//	{	
//		glVertex3f(pts[i].x, pts[i].y, pts[i].z);
//	}
//	glEnd();
//}

CVRShowModelBase::ResultFilter::~ResultFilter()
{}

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
	render = CVRender(this->model);
	initMats = CVRMats(model, viewSize);
	this->update(false);
}

void   CVRShowModelBase::setCurrentResult(const CVRResult &r)
{
	std::lock_guard<std::mutex> lock(this->_resultMutex);
	_currentResult = r;
	_hasResult = true;
}

bool   CVRShowModelBase::showCurrentResult(bool waitResult)
{
	if (!_hasResult)
	{
		if(!waitResult)
			return false;
		else
		{
			while(!_hasResult)
				std::this_thread::sleep_for(std::chrono::milliseconds(3));
		}
	}

	CVRResult r;
	{
		std::lock_guard<std::mutex> lock(this->_resultMutex);
		r = _currentResult;
		_hasResult = false;
	}

	this->present(r);

	if (this->resultFilter)
		this->resultFilter->exec(r);

	return true;
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
			wd->setCurrentResult(
				 CVRResult::blank(wd->viewSize, wd->initMats)
			);
			return;
		}

		if (!wd->render)
		{
			wd->render = CVRender(wd->model);
			if (!wd->bgImg.empty())
				wd->render.setBgImage(wd->bgImg);
		}

		//time_t beg = clock();

		auto &tb(evt.winData->trackBall);

		if (evt.code == cv::EVENT_MOUSEMOVE)
			tb.onMouseMove(evt.param1, evt.param2, theDevice.size());
		else if (evt.code == cv::EVENT_MOUSEWHEEL)
			tb.onMouseWheel(evt.param1, evt.param2, evt.data);
		else if (evt.code == cv::EVENT_KEYBOARD)
			tb.onKeyDown(evt.param1, evt.data);

		CVRMats mats = evt.winData->initMats;
		tb.apply(mats.mModel, mats.mView, true);

		{
			auto r = wd->render.exec(mats, wd->viewSize, CVRM_IMAGE | CVRM_DEPTH, wd->renderFlags, wd->userDraw.get());
			wd->setCurrentResult(r);
		}
		//postShowImage(wd);
	}
}

void _CVR_API _postShowModelEvent(const _CVRShowModelEvent &evt)
{
	if (evt.code==cv::EVENT_LBUTTONDOWN)
	{
		evt.winData->trackBall.onMouseDown(evt.param1, evt.param2);
	}
	else if (
		evt.code == cv::EVENT_MOUSEMOVE && (evt.data&cv::EVENT_FLAG_LBUTTON)
		|| evt.code == cv::EVENT_MOUSEWHEEL || evt.code == cv::EVENT_KEYBOARD
		)
	{
		evt.winData->showCurrentResult();

		cvrPost([evt](int nLeft) {
			proShowModelEvents(evt, nLeft);
		}, false);

		if (evt.code == cv::EVENT_KEYBOARD)
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
}


