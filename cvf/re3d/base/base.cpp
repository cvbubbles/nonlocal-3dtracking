
#include"base.h"
#include"BFC/fio.h"
#include"BFC/stdf.h"
#include"CVX/bfsio.h"
#include"BFC/portable.h"
#include"BFC/log.h"
#include"CVRender/cvrender.h"
using namespace ff;
#include<opencv2/calib3d.hpp>

#include"factory.h"
#include<iostream>
#include<fstream>
#include<mutex>

#if _MSC_VER>=1900
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#endif

#include<experimental/filesystem>
namespace fs = std::experimental::filesystem;


_RE3D_BEG

static std::mutex  gmutex;

#define _LOCK(mtx) std::lock_guard<std::mutex>  _lock(mtx) 
#define _GLOCK() _LOCK(gmutex)

static const char MAGIC[] = "Re3DMod\0";

class StreamSet::Impl
	:public ff::NamedStreamSet
{
public:
	//bool     hasMeshInfo = false;
	//MeshInfo meshInfo;
	std::mutex   _mtx;
public:
	Impl()
		:ff::NamedStreamSet(MAGIC)
	{}

	void Load(const char *file, bool createIfNotExist)
	{
		this->NamedStreamSet::Load(file, createIfNotExist);

		/*auto *stream = this->GetStream(MeshInfo::streamName(), false);
		if (stream)
		{
			hasMeshInfo = true;
			(*stream) >> meshInfo;
		}
		else
			hasMeshInfo = false;*/
	}
};

StreamSet::StreamSet()
	:ptr(new Impl)
{}
StreamSet::~StreamSet()
{
	delete ptr;
}

void StreamSet::load(const char *file, bool createIfNotExist)
{
	std::lock_guard<std::mutex> _lock(ptr->_mtx);
	ptr->Load(file, createIfNotExist);
}

void StreamSet::save(const char *file)
{
	std::lock_guard<std::mutex> _lock(ptr->_mtx);
	ptr->Save(file, 0);
}

const std::string& StreamSet::getFile()
{
	return ptr->GetFile();
}

StreamPtr StreamSet::getStream(const char *name, bool createIfNotExist)
{
	std::lock_guard<std::mutex> _lock(ptr->_mtx);
	auto stream=ptr->GetStream(name, createIfNotExist);
	if (!stream)
		FF_EXCEPTION(ERR_FILE_READ_FAILED, "");
	else
		stream->Seek(0, SEEK_SET);

	return stream;
}
std::string StreamSet::getStreamFile(const char *name)
{
	return ptr->GetStreamFile(name);
}

StreamPtr StreamSet::queryStream(const char *name)
{
	return ptr->GetStream(name, false);
}

void StreamSet::deleteStream(const char *name)
{
	std::lock_guard<std::mutex> _lock(ptr->_mtx);
	ptr->DeleteStream(name);
}
std::vector<std::string> StreamSet::listStreams()
{
	std::vector<std::string> names;
	ptr->ListStreams(names);
	return names;
}
//int StreamSet::nStreams()
//{
//	return ptr->Size();
//}


void CObject::release()
{
	delete this;
}
CObject::~CObject()
{}

//===============================================



void DyObject::exec(const char *cmd, const char *args, void *param1, void *param2)
{
}

static FactoryEx *gFactory = nullptr;
int   DyObject::registerType(CreateObjectFuncT createFunc, const char *group, const char *type, int flag)
{
	if (!gFactory)
		gFactory = new FactoryEx;

	gFactory->registerType(createFunc, group, type, flag != 0);

	return 0;
}

DyObject* DyObject::createObject(const char *group, const char *type)
{
	return gFactory ? gFactory->createObject<DyObject>(group, type) : nullptr;
}

//==============================================================================
#if 0
class ObjectSet::CImpl
{
public:
	static std::shared_ptr<CObject> _makePtr(CObject *ptr) {
		return std::shared_ptr<CObject>(ptr, [](CObject *ptr) {ptr->release(); });
	}

	struct DObject
	{
		std::string name;
		std::shared_ptr<CObject>  ptr;
	public:
		DObject()
		{}
		DObject(const char *_name, CObject *_ptr)
			:name(_name),
			ptr(_makePtr(_ptr))
		{}
	};
	std::list<DObject>  objList;
	//std::mutex   _mutex;
	std::recursive_mutex _mutex;

	typedef std::list<DObject>::iterator _ItrT;

	_ItrT findObject(const char *name)
	{
		auto itr = objList.begin();
		for (; itr != objList.end(); ++itr)
			if (strcmp(itr->name.c_str(), name) == 0)
				break;
		return itr;
	}
	CObject* getObject(ObjectSet *site, const char *name)
	{
		std::lock_guard<std::recursive_mutex> _lock(_mutex);

		auto itr = this->findObject(name);
		if(itr!=objList.end())
			return itr->ptr.get();

		auto *ptr = site->_createObject(name);
		if (ptr)
			objList.push_back(DObject(name, ptr));
		
		return ptr;
	}
	void setObject(const char *name, CObject *obj) {
		
		std::lock_guard<std::recursive_mutex> _lock(_mutex);

		auto itr = this->findObject(name);
		if (itr != objList.end())
			itr->ptr = _makePtr(obj);
		else
			objList.push_back(DObject(name, obj));
	}
};

#else

class ObjectSet::CImpl
{
public:
	struct DObject
	{
		std::string name;
		Variable    var;
		bool        isManaged = false;
	public:
		DObject()
		{}
		void destroy()
		{
			if (isManaged)
			{
				CObject *ptr = var.get<CObject*>();
				ptr->release();
				isManaged = false;
			}
		}
		DObject(const char *_name, const Variable &_var, bool _isManaged)
			:name(_name),var(_var),isManaged(_isManaged)
		{}
	};
	std::list<DObject>  objList;
	//std::mutex   _mutex;
	std::recursive_mutex _mutex;

	typedef std::list<DObject>::iterator _ItrT;

	~CImpl()
	{
		for (auto &v : objList)
			v.destroy();
	}

	_ItrT findObject(const char *name)
	{
		auto itr = objList.begin();
		for (; itr != objList.end(); ++itr)
			if (strcmp(itr->name.c_str(), name) == 0)
				break;
		return itr;
	}
	bool remove(const char *name)
	{
		_ItrT itr = this->findObject(name);
		if (itr != objList.end())
		{
			itr->destroy();
			objList.erase(itr);
			return true;
		}
		return false;
	}

	Variable* getObject(ObjectSet *site, const char *name, bool isQuery)
	{
		std::lock_guard<std::recursive_mutex> _lock(_mutex);

		auto itr = this->findObject(name);
		if (itr != objList.end())
			return &itr->var;

		CObject *ptr = site->_createObject(name);
		if (!ptr)
		{
			if (isQuery)
				return nullptr;
			else
				throw std::invalid_argument((std::string("unknown variable ") + name).c_str());
		}
		objList.push_back(DObject(name, Variable(ptr),true));

		return &objList.back().var;
	}
	Variable& make(ObjectSet *site, const char *name)
	{
		std::lock_guard<std::recursive_mutex> _lock(_mutex);

		Variable *ptr = this->getObject(site, name, false);
		if (!ptr)
		{
			objList.push_back(DObject(name, Variable(),false));
			ptr = &objList.back().var;
		}
		return *ptr;
	}
};

#endif

ObjectSet::ObjectSet()
	:iptr(new CImpl)
{}
Variable* ObjectSet::query(const char *name)
{
	return iptr->getObject(this, name, true);
}
Variable& ObjectSet::get(const char *name)
{
	return *iptr->getObject(this, name, false);
}
Variable& ObjectSet::make(const char *name)
{
	return iptr->make(this, name);
}
bool ObjectSet::remove(const char *name)
{
	return iptr->remove(name);
}

CObject* ObjectSet::_createObject(const char *name)
{
	return nullptr;
}
static std::mutex   _modelMutex;

class Model::CImpl
{
public:
	ModelInfos   _infos;

	CVRModel     _model3D;
	StreamSet    *_StreamSet=nullptr;

public:
	CImpl(/*ModelSet *modelSet*/)
		//:_modelSet(modelSet)
	{
		//_args.setNext(&modelSet->getArgSet());
	}
	~CImpl()
	{
		this->_clearLoaded();
	}
	void _clearLoaded()
	{
		_model3D = CVRModel();
		
		delete _StreamSet;
		_StreamSet = nullptr;
	}
	void set(const ModelInfos &infos)
	{
		_LOCK(_modelMutex);

		_infos = infos;
		this->_clearLoaded();
	}
	
	CVRModel& get3D() {
		_LOCK(_modelMutex);
		if (!_model3D)
		{
			LOG("Loading 3D model file for '%s'\n", _infos.uniqueID.c_str());

			std::string options = "";
			_model3D.load(_infos.modelFile, 3, options);
		}
		return _model3D;
	}
	
	StreamSet& getData(bool createIfNotExist) {
		_LOCK(_modelMutex);
		if (!_StreamSet)
		{
			_StreamSet = new StreamSet;
			_StreamSet->load(_infos.dataFile.c_str(), createIfNotExist);
		}
		return *_StreamSet;
	}
};

Model::Model(/*ModelSet *modelSet*/)
	:iptr(new CImpl(/*modelSet*/))
{}

Model::~Model()
{
	delete iptr;
}

void Model::set(const ModelInfos &infos)
{
	iptr->set(infos);
}

CVRModel& Model::get3DModel()
{
	return iptr->get3D();
}
//const std::string& Model::get3DModelFile()
//{
//	return iptr->_infos.modelFile;
//}
StreamSet& Model::getData(bool createIfNotExist)
{
	return iptr->getData(createIfNotExist);
}

const ModelInfos& Model::getInfos()
{
	return iptr->_infos;
}

CObject* Model::_createObject(const char *name)
{
	auto *ptr = ManagedObject::createObjectT<ManagedObject>(ManagedObject::getGroupName(),name);
	if(ptr)
		ptr->onCreate(*this);
	return ptr;
}

//================================================

class ModelFactory::CImpl
{
public:
	struct _Model
	{
		ModelPtr  ptr;
	};

	typedef std::map<std::string, _Model> _MapT;

	_MapT  _models;
	std::mutex  _mutex;
public:

	ModelPtr  getModel(const ModelInfos &infos)
	{
		std::lock_guard<std::mutex> _lock(_mutex);

		auto itr = _models.find(infos.uniqueID);
		if (itr == _models.end())
		{
			std::string myBufDir = ff::GetDirectory(infos.dataFile);
			if (!ff::pathExist(myBufDir))
			{
				ff::makeDirectory(myBufDir);

				std::string infoFile = myBufDir + "/info.txt";
				std::ofstream os(infoFile);
				os << infos.modelFile << std::endl;
			}
			if (!ff::pathExist(myBufDir))
				FF_EXCEPTION1(ff::StrFormat("failed to create buf directory for model %s\n", infos.uniqueID.c_str()));


			ModelPtr ptr(new Model);
			ptr->set(infos);

			auto pr = _models.emplace(infos.uniqueID, _Model());
			itr = pr.first;
			itr->second.ptr = ptr;
		}

		return itr->second.ptr;
	}
};

ModelFactory::ModelFactory()
	:iptr(new CImpl)
{}
ModelFactory::~ModelFactory()
{
	delete iptr;
}

//void  ModelFactory::setBufferDir(const std::string &bufDir)
//{
//	iptr->_bufDir = bufDir;
//}
ModelPtr ModelFactory::getModel(const ModelInfos &infos)
{
	return iptr->getModel(infos);
}

//================================================

static std::mutex  g_modelsetMutex;
class ModelSet::CImpl
{
public:
	struct _ModelInfosEx
		:public ModelInfos
	{
		ModelPtr  ptr;
	public:
		ModelPtr get()
		{
			if (!ptr)
				ptr = app()->modelFactory.getModel(*this);
			return ptr;
		}
	};

	ModelSetInfos                _infos;
	std::vector<_ModelInfosEx>   _models;
	StreamSet                   *_streamSet=nullptr;
public:
	~CImpl()
	{
		this->_clear();
	}
	void _clear()
	{
		_models.clear();

		if (_streamSet)
		{
			delete _streamSet;
			_streamSet = nullptr;
		}
	}
	StreamSet& getData(bool createIfNotExist)
	{
		_LOCK(g_modelsetMutex);
		if (!_streamSet)
		{
			_streamSet = new StreamSet;
			_streamSet->load(_infos.dataFile.c_str(), createIfNotExist);
		}
		return *_streamSet;
	}
	void set(const ModelSetInfos &infos)
	{
		this->_clear();
		_models.resize(infos.models.size());
		for (size_t i = 0; i < infos.models.size(); ++i)
		{
			static_cast<ModelInfos&>(_models[i]) = infos.models[i];
		}
		_infos = infos;
	}
	int _findModel(const std::string &name)
	{
		for (int i = 0; i < (int)_models.size(); ++i)
			if (_models[i].name == name)
				return i;
		return -1;
	}
	ModelPtr get(int idx) {
		if (uint(idx) >= _models.size() )
			return nullptr;

		return _models[idx].get();
	}
	 ModelPtr get(const std::string &name) {
		 return this->get(this->_findModel(name));
	}
	/* const std::vector<ModelInfosEx>& getAll() const
	 {
		 static_assert(sizeof(_ModelInfosEx) == sizeof(ModelInfosEx), "");
		 return reinterpret_cast<const std::vector<ModelInfosEx>&>(this->_models);
	}*/
};

ModelSet::ModelSet()
	:iptr(new CImpl)
{}
ModelSet::~ModelSet()
{
	delete iptr;
}


CObject* ModelSet::_createObject(const char *name)
{
	auto *ptr = ManagedObject::createObjectT<ManagedObject>(ManagedObject::getGroupName(), name);
	if(ptr)
		ptr->onCreate(*this);
	return ptr;
}
void ModelSet::set(const ModelSetInfos &infos)
{
	iptr->set(infos);
}

void ModelSet::set(const std::vector<ModelInfos> &models, const std::string &owner, const std::string &name)
{
	this->set(app()->configModelSet(models, owner, name));
}

int  ModelSet::size()
{
	return (int)iptr->_models.size();
}
ModelPtr ModelSet::getModel(int idx) 
{
	return iptr->get(idx);
}

ModelPtr ModelSet::getModel(const std::string &name)
{
	return iptr->get(name);
}

int  ModelSet::getModelIndex(Model *p)
{
	auto &models = iptr->_models;
	for (int i = 0; i < (int)models.size(); ++i)
		if (models[i].ptr.get() == p)
			return i;
	return -1;
}

int  ModelSet::getModelIndex(const std::string &name)
{
	return iptr->_findModel(name);
}

//Model& ModelSet::operator[](int idx) {
//	auto p = this->getModel(idx);
//	if (!p)
//		FF_EXCEPTION(ERR_INVALID_ARGUMENT, "");
//	return *p;
//}
//Model& ModelSet::operator[](const std::string &name) {
//	auto p = this->getModel(name);
//	if (!p)
//		FF_EXCEPTION(ERR_INVALID_ARGUMENT, "");
//	return *p;
//}
const ModelSetInfos& ModelSet::getInfos()
{
	return iptr->_infos;
}

StreamSet& ModelSet::getData(bool createIfNotExist)
{
	return iptr->getData(createIfNotExist);
}


_RE3D_API std::vector<ModelInfos>  modelInfosFromFiles(const std::vector<std::string>  &modelFiles, const std::string &owner)
{
	std::vector<ModelInfos> infos;
	for (auto &f : modelFiles)
		infos.push_back(app()->configModel(f, owner));
	return infos;
}

_RE3D_API std::vector<ModelInfos>  modelInfosFromDir(const std::string &modelDir, const std::string &owner, bool searchRecursive, const std::string &filter)
{
	std::vector<std::string> files;
	ff::listFiles(modelDir, files, searchRecursive);

	std::vector<std::string>  fullPath;
	for (auto &f : files)
	{
		std::string ext = ff::GetFileExtention(f);
		if (filter.find(ext, 0) != std::string::npos)
		{
			fullPath.push_back(ff::CatDirectory(modelDir, f));
		}
	}
	return modelInfosFromFiles(fullPath, owner);
}

_RE3D_API std::vector<ModelInfos>  modelInfosFromListFile(const std::string &listFile, const std::string &owner)
{
	std::ifstream is(listFile);
	if (!is)
		FF_EXCEPTION1("failed to open model list file " + listFile);

	std::vector<ModelInfos> infos;
	std::string dir = ff::GetDirectory(listFile);
	std::string name, file;
	while (is >> name >> file)
	{
		infos.push_back(
			app()->configModel(dir + "/" + file, owner, name)
		);
	}
	return infos;
}



class App::CImpl
{
public:
	ff::ArgSet  args;
	std::string tempDir;
	/*std::string modelDataDir;
	std::string modelSetDataDir;*/
};

App::App()
	:iptr(new CImpl)
{}
App::~App()
{
	delete iptr;
}
CObject* App::_createObject(const char *name)
{
	auto *ptr = ManagedObject::createObjectT<ManagedObject>(ManagedObject::getGroupName(), name);
	if (ptr)
		ptr->onCreate(*this);
	return ptr;
}
App* App::instance()
{
	_GLOCK();

	static App *_app = nullptr;
	if (!_app)
		_app = new App();
	return _app;
}
void App::setTempDir(const std::string &dir)
{
	iptr->tempDir = dir;
}
const std::string& App::getTempDir()
{
	if (iptr->tempDir.empty())
		FF_EXCEPTION1("TempDir is not set, call App::setTempDir first");
	return iptr->tempDir;
}
std::string App::getOwnerDataDir(const std::string &owner)
{
	return getTempDir() + "/owners/" + owner + "/";
}

static std::string _encodeBytesAsID(const char *bytes, int size)
{
	if (!bytes || size == 0)
		return "00000000-00000000";

	std::string str;
	str.resize(size);
	memcpy(&str[0], bytes, size);

	enum { USIZE = sizeof(uint) };
	const int codeLen = (size / USIZE + 1)*USIZE;
	for (int i = size, j = 0; i < codeLen + USIZE; ++i, ++j)
		str.push_back(str[j]);

	const uint *p = (const uint*)str.data();
	uint v1 = 0, v2 = 0;
	int n = codeLen / USIZE;
	for (int i = 0; i < n; ++i)
		v1 += p[i];

	p = (const uint*)&str[USIZE / 2];
	for (int i = 0; i < n; ++i)
		v2 += p[i];

	char vid[32];
	sprintf(vid, "%08X-%08X", v1, v2);
	return vid;
}

static std::string getFileVersionCode(const std::string &file)
{
	
	fs::path filePath(file);
	auto time = fs::last_write_time(filePath);
	auto size = fs::file_size(filePath);
	char buf[sizeof(time) + sizeof(size)] = { 0 };
	memcpy(buf, &time, sizeof(time));
	memcpy(&buf[sizeof(time)], &size, sizeof(size));
	return _encodeBytesAsID(buf, sizeof(buf));
}

const std::string DEFAULT_OWNER = "_default";

std::string getDefaultModelName(const std::string& fullPath)
{
	std::string name = ff::GetFileName(fullPath, false);
	std::string dirName = ff::GetFileName(ff::GetDirectory(fullPath));

	std::string pathSmall = fullPath;
	ff::str2lower(pathSmall);

	std::string pathCode = _encodeBytesAsID(pathSmall.c_str(), pathSmall.size());
	return (dirName.empty() ? name : dirName + "." + name) + "[" + pathCode + "]";
}

ModelInfos  App::configModel(const std::string &modelFile, const std::string &owner, const std::string &name)
{
	std::string fullModelFile = ff::getFullPath(modelFile);

	ModelInfos mi;
	mi.name = !name.empty() ? name : getDefaultModelName(fullModelFile);
	mi.owner = !owner.empty() ? owner : DEFAULT_OWNER;
	mi.modelFile = fullModelFile;

	/*std::string str = fullModelFile;
	ff::str2lower(str);
	mi.uniqueID = _encodeStringAsID(str) + "[" + mi.name + "]";*/
	mi.uniqueID = mi.owner + "." + mi.name;
	mi.versionCode = getFileVersionCode(mi.modelFile);

	std::string myBufDir = this->getOwnerDataDir(mi.owner) + "/model_data/" + mi.name + "/";
#if 0
	if (!ff::pathExist(myBufDir))
	{
		ff::makeDirectory(myBufDir);

		std::string infoFile = myBufDir + "/info.txt";
		std::ofstream os(infoFile);
		os << ff::getFullPath(fullModelFile) << std::endl;
	}
	if (!ff::pathExist(myBufDir))
		FF_EXCEPTION1(ff::StrFormat("failed to create buf directory for model %s\n", mi.uniqueID.c_str()));
#endif

	mi.dataFile = myBufDir + "/v002.mdb";
	return mi;
}
ModelSetInfos App::configModelSet(const std::vector<ModelInfos> &models, const std::string &owner, const std::string &name)
{
	ModelSetInfos msi;
	msi.name = !name.empty() ? name : "default";
	msi.owner = !owner.empty() ? owner : DEFAULT_OWNER;

	{
		std::vector<const std::string*>  vids;
		for (auto &m : models)
			vids.push_back(&m.versionCode);
		std::sort(vids.begin(), vids.end(), [](const std::string *a, const std::string *b) {
			return *a < *b;
		});
		std::string codeStr;
		for (auto str : vids)
			codeStr += *str;
		msi.versionCode = _encodeBytesAsID(codeStr.data(),(int)codeStr.size());
	}
	msi.uniqueID = msi.owner + "." + msi.name;

	std::string myBufDir = this->getOwnerDataDir(msi.owner) + "/modelset_data/" + msi.name + "/";
	msi.dataFile = myBufDir + "/v001.mdb";
	msi.models = models;
	return msi;
}



//const std::string& App::getModelDataDir()
//{
//	if (iptr->modelDataDir.empty())
//		iptr->modelDataDir = getTempDir() + "/model_data/";
//	return iptr->modelDataDir;
//}
//const std::string& App::getModelSetDataDir()
//{
//	if (iptr->modelSetDataDir.empty())
//		iptr->modelSetDataDir = getTempDir() + "/modelset_data/";
//	return iptr->modelSetDataDir;
//}
ff::ArgSet& App::args()
{
	return iptr->args;
}
void App::setArgs(const char *args)
{
	_GLOCK();

	iptr->args.setArgs(args);
}



RigidPose::RigidPose(const cv::Vec3f &rvec, const cv::Vec3f &tvec)
	:t(tvec)
{
	cv::Mat _R;
	cv::Rodrigues(rvec, _R);
	this->R = _R;
}

RigidPose::RigidPose(const cv::Mat &R_or_rvec, const cv::Mat &tvec)
{
	if (!R_or_rvec.empty())
	{
		int n = R_or_rvec.rows*R_or_rvec.cols;
		CV_Assert(R_or_rvec.size() == Size(3, 3) || n == 3);
		if (n == 3)
		{
			cv::Mat _R;
			cv::Rodrigues(R_or_rvec, _R);
			this->R = _R;
		}
		else
			this->R = R_or_rvec;
	}
	else
		this->R = cv::Matx33f::eye();

	if (!tvec.empty())
	{
		CV_Assert(tvec.rows*tvec.cols == 3);
		this->t = tvec;
	}
	else
		this->t = cv::Vec3f(0.f, 0.f, 0.f);
}
RigidPose::RigidPose(const cv::Vec6f& rtVec)
	:t(rtVec[3],rtVec[4],rtVec[5])
{
	cv::Mat _R;
	cv::Rodrigues(cv::Vec3f(rtVec[0], rtVec[1], rtVec[2]), _R);
	this->R = _R;
}
cv::Vec6f RigidPose::getVec6() const
{
	auto r = this->getRvec();
	return cv::Vec6f(r[0], r[1], r[2], t[0], t[1], t[2]);
}

cv::Vec3f RigidPose::getRvec() const
{
	cv::Mat Rvec;
	cv::Rodrigues(this->R, Rvec);
	CV_Assert(Rvec.cols * Rvec.rows == 3 && Rvec.type() == CV_32FC1);
	const float* v = Rvec.ptr<float>();
	return cv::Vec3f(v[0], v[1], v[2]);
}

//=========================================================================
#if 0
static const int CUR_VERSION = 200;

template<typename _StreamT>
void BFSRead(_StreamT &stream, MeshInfo &info)
{
	int version;
	stream >> version;
	info.version = version;

	if(version==100)
		stream >> info.center >> info.bbMin >> info.bbMax;
	else if(version==200)
		stream >> info.center >> info.bbMin >> info.bbMax >> info.R0 >> info.bbSizeR0;
	else
		FF_EXCEPTION1("invalid version number of MeshInfo");
}

template<typename _StreamT>
void BFSWrite(_StreamT &stream, const MeshInfo &info)
{
	//int version = 200;
	stream << info.version;

	stream << info.center << info.bbMin << info.bbMax;
	stream << info.R0 << info.bbSizeR0;
}

static void _setMeshInfoV100(Model &model, MeshInfo &mi)
{
	auto &mesh = model.get3D();
	mi.center = mesh.getCenter();
	mesh.getBoundingBox(mi.bbMin, mi.bbMax);
	mi.version = 100;
}
using namespace cv;
static void _getBoundingBox(const std::vector<Vec3f> &vtx, Vec3f &bbMin, Vec3f &bbMax)
{
	if (vtx.empty())
		return;

	bbMin = bbMax = vtx[0];
	for (auto &v : vtx)
	{
		for (int i = 0; i < 3; ++i)
		{
			if (v[i] < bbMin[i])
				bbMin[i] = v[i];
			else if (v[i] > bbMax[i])
				bbMax[i] = v[i];
		}
	}
}
static void _setMeshInfoV200(Model &model, MeshInfo &mi)
{
	mi.version = 200;
	auto &mesh = model.get3D();

	const std::vector<Vec3f> &vtx = mesh.getVertices();

	Mat mvtx(vtx.size(), 3, CV_32FC1, (void*)&vtx[0]);
	cv::PCA pca(mvtx, noArray(), PCA::DATA_AS_ROW);

	Vec3f mean = pca.mean;
	Matx33f ev = pca.eigenvectors;

	Matx44f R;
	{
		Vec3f vx(ev(1, 0), ev(1, 1), ev(1, 2));
		Vec3f vy(ev(0, 0), ev(0, 1), ev(0, 2));
		Vec3f vz = vx.cross(vy);
		R = cvrm::rotate(&vx[0], &vy[0], &vz[0]);
	}
	CV_Assert(cv::determinant(R) > 0.f);

	mi.center = mean;
	_getBoundingBox(vtx,mi.bbMin, mi.bbMax);
	mi.R0 = R;

	Matx44f mT = cvrm::translate(-mean[0], -mean[1], -mean[2])*R;

	using ::operator*; //operator* in cvrender.h

	std::vector<Vec3f> vtxT;
	vtxT.reserve(vtx.size());
	for (auto &v : vtx)
		vtxT.push_back(v*mT);

	Vec3f bbMin, bbMax;
	_getBoundingBox(vtxT, bbMin, bbMax);

	mi.bbSizeR0 = bbMax - bbMin;
}

void MeshInfo::onCreate(Model &model)
{
	static const char *streamName = "MeshInfo";

	auto &StreamSet = model.getData();
	auto &stream = StreamSet.getStream(streamName, true);
	if (!stream.Empty())
		stream >> *this;

	if (stream.Empty()||version!= CUR_VERSION)
	{
		_setMeshInfoV200(model, *this);
		stream << *this;
		StreamSet.save();
	}
}
Matx44f MeshInfo::getUnitize() const
{
	return CVRModel::getUnitize(center, bbMin, bbMax);
}
void MeshInfo::getFrameR0(cv::Point3f oxyz[4])
{
	oxyz[0] = center;

	Vec3f vscales = bbSizeR0*0.5f;

	cv::Matx44f Rx = R0.inv();
	oxyz[1] = oxyz[0] + Point3f(1.f, 0.f, 0.f)*Rx*vscales[0];
	oxyz[2] = oxyz[0] + Point3f(0.f, 1.f, 0.f)*Rx*vscales[1];
	oxyz[3] = oxyz[0] + Point3f(0.f, 0.f, 1.f)*Rx*vscales[2];
}

REGISTER_RE3D_TYPE(MeshInfo, 0)
#endif

CObject* FrameData::_createObject(const char *name)
{
	auto *ptr=ManagedObject::createObjectT<ManagedObject>(ManagedObject::getGroupName(), name);
	return ptr;
}


_RE3D_END


#ifdef _MSC_VER
#include<Windows.h>

_RE3D_BEG

_RE3D_API  void* loadModule(const std::string &libFile)
{
	HMODULE hmod=::LoadLibraryA(libFile.c_str());
	if (!hmod)
		FF_EXCEPTION1(ff::StrFormat("failed to load library %s\n", libFile.c_str()));

	return hmod;
}

_RE3D_END

#else //linux implementation

_RE3D_BEG

#include<dlfcn.h>

_RE3D_API  void* loadModule(const std::string &libFile)
{
	void *hmod = dlopen(libFile.c_str(), RTLD_LAZY);
    if (!hmod) 
        FF_EXCEPTION1(ff::StrFormat("failed to load library %s\n", libFile.c_str()));
	return hmod;
}

_RE3D_END

#endif
