
#include"DatasetRender.h"

#include"pybind11/pybind11.h"

#include"my_converter.h"
using namespace pybind11::literals;

#include<iostream>
using namespace std;

void init(const char *args)
{ 
	cvrInit(args);
} 

Matx44f test_matx(const Matx44f &m, CVRMats mats)
{
	return mats.mModel;
}

static pybind11::tuple decomposeRT(const Matx44f &m)
{
	Vec3f rv, tv;
	cvrm::decomposeRT(m, rv, tv);
	return pybind11::make_tuple(rv, tv);
}
static pybind11::tuple decomposeR33T(const Matx44f &m)
{
	Matx33f R;
	Vec3f t;
	cvrm::decomposeRT(m, R, t);
	return pybind11::make_tuple(R, t);
}
static std::vector<Vec3f> sampleSphere(int n)
{
	std::vector<Vec3f> v;
	cvrm::sampleSphere(v, n);
	return v;
}

static void py_mdshow(const std::string &wndName, const CVRModel &model, Size viewSize = Size(800, 800), int renderFlags = CVRM_DEFAULT, const cv::Mat bgImg = cv::Mat())
{
	mdshow(wndName, model, viewSize, renderFlags, bgImg);
}

static Mat1b getRenderMask(const Mat1f &depth, float eps = 1e-6f)
{
	Mat1b mask = Mat1b::zeros(depth.size());
	for_each_2(DWHN1(depth), DN1(mask), [eps](float d, uchar &m) {
		m = fabs(1.0f - d)<eps ? 0 : 255;
	});
	return mask;
}

static Mat4b postProRender(const Mat3b &img, const Mat1b &mask)
{
	Mat3b F = img.clone();
	smoothObject(F, mask);
	Mat1b dmask;
	GaussianBlur(mask, dmask, Size(3, 3), 1.0);

	Mat4b C(F.size());
	for_each_3(DWHNC(F), DN1(dmask), DNC(C), [](const uchar *F, uchar a, uchar *C) {
		C[0] = F[0]; C[1] = F[1]; C[2] = F[2]; C[3] = a;
	});
	return C;
}

static pybind11::tuple cropImageRegion(const Mat &img, const Mat1b &mask, int borderWidth=0, Vec2i offset=Vec2i(0,0))
{
	Rect roi = cv::get_mask_roi(DWHS(mask), 127);
	//cout << roi << endl;

	cv::rectAppend(roi, borderWidth, borderWidth, borderWidth, borderWidth);
	roi.x+=offset[0];
	roi.y+=offset[1];

	roi = rectOverlapped(roi, Rect(0, 0, img.cols, img.rows));
	//cout << roi << endl;

	return pybind11::make_tuple(
		img(roi).clone(), mask(roi).clone()
	);
}
 
#include"BFC/err.h"
#include<exception>

static void  MyErrorHandler(int type, const char *err, const char *msg, const char* file, int line)
{
	if (type == ERROR_CLASS_EXCEPTION)
	{
		auto msgStr=ff::StrFormat("%s:%s", err ? err : "", msg ? msg : "");
		throw std::domain_error(msgStr.c_str());
	}
	else
		ff::DefaultErrorHandler(type, err, msg, file, line);
}

PYBIND11_MODULE(cvrender, m) {

	//py::register_exception<ff::CSException>(m, "RuntimeError");
	ff::SetErrorHandler(MyErrorHandler);

	NDArrayConverter::init_numpy();

	m.def("init", init, "init the module");

	m.def("test_matx", test_matx, "test...");

	//export cvrm functions
	m.def("diag", cvrm::diag);
	m.def("I", cvrm::I);
	m.def("translate", cvrm::translate);
	m.def("scale", (Matx44f(*)(float,float,float))cvrm::scale);
	m.def("rotateAngle", (Matx44f(*)(float, const cv::Vec3f &)) cvrm::rotate, "rotate angle around an axis");
	m.def("rotateVecs", (Matx44f(*)(const cv::Vec3f &, const cv::Vec3f &)) cvrm::rotate, "get rotate between two vectors");
	m.def("ortho", cvrm::ortho,"Matx44f ortho(float left, float right, float bottom, float top, float nearP, float farP)");
	m.def("perspectiveF", (Matx44f (*)(float f, Size windowSize, float nearP, float farP)) cvrm::perspective, "Matx44f perspective(float f, Size windowSize, float nearP, float farP)");
	m.def("perspectiveK",(Matx44f (*)(float fx, float fy, float cx, float cy, Size windowSize, float nearP, float farP))cvrm::perspective,"Matx44f perspective(float fx, float fy, float cx, float cy, Size windowSize, float nearP, float farP)");
	m.def("defaultK", cvrm::defaultK);
	m.def("fromK", cvrm::fromK);
	m.def("fromRT", cvrm::fromRT);
	m.def("fromR33T", cvrm::fromR33T);
	m.def("lookat", cvrm::lookat); 
	m.def("decomposeRT", decomposeRT);
	m.def("decomposeR33T", decomposeR33T);
	m.def("project", cvrm::project);
	m.def("unproject", cvrm::unproject);
	m.def("sampleSphere", sampleSphere);
	m.def("rot2Euler",cvrm::rot2Euler);
	m.def("euler2Rot",cvrm::euler2Rot);
	
	m.def("getRenderMask", getRenderMask, "depth"_a, "eps"_a = 1e-6f);
	m.def("postProRender", postProRender);
	m.def("cropImageRegion", cropImageRegion, "img"_a, "mask"_a, "borderWidth"_a = 0, "offset"_a=Vec2i(0,0));

	m.def("mdshow", py_mdshow, "wndName"_a, "model"_a, "viewSize"_a = Size(800, 800), "renderFlags"_a = (int)CVRM_DEFAULT, "bgImg"_a = cv::Mat());
	m.def("waitKey", cv::cvxWaitKey, "exitCode"_a = (int)cv::KEY_ESCAPE);

	//exports constants
	m.attr("CVRM_IMAGE") = (int)CVRM_IMAGE;
	m.attr("CVRM_DEPTH") = (int)CVRM_DEPTH;

	m.attr("CVRM_ENABLE_LIGHTING") = (int)CVRM_ENABLE_LIGHTING;
	m.attr("CVRM_ENABLE_TEXTURE") = (int)CVRM_ENABLE_TEXTURE;
	m.attr("CVRM_ENABLE_MATERIAL") = (int)CVRM_ENABLE_MATERIAL;
	m.attr("CVRM_TEXTURE_NOLIGHTING") = (int)CVRM_TEXTURE_NOLIGHTING;
	m.attr("CVRM_ENABLE_ALL") = (int)CVRM_ENABLE_ALL;
	m.attr("CVRM_TEXCOLOR") = (int)CVRM_TEXCOLOR;
	m.attr("CVRM_DEFAULT") = (int)CVRM_DEFAULT;

	//exports other classes
	py::class_<CVRRendable>(m, "CVRRendable")
		;

	py::class_<CVRModel, CVRRendable>(m, "CVRModel")
		.def(py::init<>())
		.def(py::init<const std::string&>())
		.def("load", &CVRModel::load, "file"_a, "postProLevel"_a = 3, "options"_a = "")
		.def("saveAs",&CVRModel::saveAs,"file"_a,"fmtID"_a="","options"_a="-std")
		.def("getCenter",&CVRModel::getCenter)
		.def("getVertices",&CVRModel::getVertices)
		.def("getSizeBB",&CVRModel::getSizeBB)
		.def("getFile",&CVRModel::getFile)
		.def("transform",&CVRModel::transform)
		.def("estimatePose0",&CVRModel::estimatePose0)
		.def("getBoundingBox",&CVRModel::__getBoundingBox)
		; 

	py::class_<CVRMats>(m, "CVRMats")
		.def(py::init<>())
		.def(py::init<cv::Size,float,float,float,float>(),"viewSize"_a,"fscale"_a=1.5f,"eyeDist"_a=4.0f,"zNear"_a=0.1,"zFar"_a=100)
		.def(py::init<CVRModel,cv::Size, float, float, float, float>(), "model"_a,"viewSize"_a, "fscale"_a = 1.5f, "eyeDist"_a = 4.0f, "zNear"_a = 0.1, "zFar"_a = 100)
		.def("setInImage",&CVRMats::setInImage,"model"_a,"viewSize"_a,"K"_a,"sizeScale"_a=1.0f,"zNear"_a=1.0f,"zFar"_a=1.0f)
		.def("setInROI",&CVRMats::setInROI, "model"_a, "viewSize"_a, "roi"_a, "K"_a, "sizeScale"_a = 1.0f, "zNear"_a = 1.0f, "zFar"_a = 1.0f)
		.def("modelView",&CVRMats::modelView)
		.def_readwrite("mModeli", &CVRMats::mModeli)
		.def_readwrite("mModel", &CVRMats::mModel)
		.def_readwrite("mView", &CVRMats::mView)
		.def_readwrite("mProjection", &CVRMats::mProjection)
		;

	py::class_<CVRResult>(m, "CVRResult")
		.def_readwrite("img", &CVRResult::img)
		.def_readwrite("depth", &CVRResult::depth)
		.def_readwrite("mats", &CVRResult::mats)
		.def_readwrite("outRect", &CVRResult::outRect)
		;
  
	py::class_<CVRender>(m, "CVRender") 
		.def(py::init<>())
		.def(py::init<CVRRendable&>())
		.def("empty", &CVRender::empty) 
		.def("setBgImage", &CVRender::setBgImage)
		.def("clearBgImage", &CVRender::clearBgImage)
		.def("setBgColor", &CVRender::setBgColor)
		//CVRMats &mats, Size viewSize, int output = CVRM_IMAGE | CVRM_DEPTH, int flags = CVRM_DEFAULT, cv::Rect outRect = cv::Rect(0, 0, 0, 0)
		.def("exec", &CVRender::__exec,"mats"_a,"viewSize"_a,"output"_a= (int)CVRM_IMAGE | CVRM_DEPTH,"flags"_a= (int)CVRM_DEFAULT,"outRect"_a=cv::Rect(0,0,0,0))
		;
 
	py::class_<DatasetRender>(m, "DatasetRender")
		.def(py::init<>())
		.def("loadModels", &DatasetRender::loadModels)
		.def("getModels",&DatasetRender::getModels)
		.def("getBOPModelInfo",&DatasetRender::getBOPModelInfo)
		.def("getRenders",&DatasetRender::getRenders)
		.def("render", &DatasetRender::render)
		.def("renderToImage", &DatasetRender::renderToImage, "img"_a, "models"_a,
			"centers"_a = std::vector<std::array<int, 2>>(), "sizes"_a = std::vector<int>(),
			"viewDirs"_a = std::vector<std::array<float, 3>>(), "inPlaneRotations"_a = std::vector<float>(),
			"alphaScales"_a=std::vector<float>(),
			"harmonizeF"_a=true,
			"degradeF"_a=true,"maxSmoothSigma"_a=1.0f, "maxNoiseStd"_a=5.0f
		);

	py::class_<Compositer>(m,"Compositer")
		.def(py::init<>())
		.def("init",&Compositer::init,"bgImg"_a,"dsize"_a=cv::Size(-1,-1))
		.def("addLayer",&Compositer::addLayer,"objImg"_a,"objMask"_a,"objROI"_a,"objID"_a=-1,"harmonizeF"_a=true,"degradeF"_a=true,"maxSmoothSigma"_a=1.0f,"maxNoiseStd"_a=5.0f,"alphaScale"_a=1.0f)
		.def("getMaskOfObjs",&Compositer::getMaskOfObjs)
		.def("getComposite",&Compositer::getComposite)
		;
}

//PYBIND11_PLUGIN(pyre3d)
//{
//	NDArrayConverter::init_numpy();
//
//	py::module m("example", "pybind11 opencv example plugin");
//	m.def("read_image", &read_image, "A function that read an image",
//		py::arg("image"));
//
//	m.def("show_image", &show_image, "A function that show an image",
//		py::arg("image"));
//
//	m.def("passthru", &passthru, "Passthru function", py::arg("image"));
//	m.def("clone", &cloneimg, "Clone function", py::arg("image"));
//
//	return m.ptr();
//}



