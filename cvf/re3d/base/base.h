#pragma once

#include"re3d/base/def.h"
#include"opencv2/core/mat.hpp"
#include<memory>
#include<typeinfo>
#include<stdexcept>
#include"BFC/err.h"

namespace ff {
	class BMStream;

	class BFStream;

	class ArgSet;
}

class CVRModel;

_RE3D_BEG

/*Managing a set of streams for storing model/model-set data
*/
typedef std::shared_ptr<ff::BFStream> StreamPtr;

class _RE3D_API StreamSet
{
	class Impl;
	Impl *ptr;
public:
	StreamSet();

	~StreamSet();

	StreamSet(const StreamSet &) = delete;
	StreamSet& operator=(const StreamSet&) = delete;

	void load(const char *file, bool createIfNotExist = false);
	
	void save(const char *file=nullptr);

	const std::string& getFile();

	//may return NULL
	StreamPtr queryStream(const char *name);

	//force success, throw exception if failed
	StreamPtr getStream(const char *name, bool createIfNotExist = false);

	std::string getStreamFile(const char *name);

	void deleteStream(const char *name);

	std::vector<std::string> listStreams();

	//int  nStreams();
};

//_RE3D_API void exec(const char *cmd);
//
//inline void exec(const std::string &cmd) {
//	exec(cmd.c_str());
//}

class _RE3D_API CObject
{
public:
	virtual void release();

	virtual size_t getTypeID() {
		return typeid(*this).hash_code();
	}

	virtual ~CObject();
};

typedef void* (*CreateObjectFuncT)();

//object type that supports dynamic-creation (create with string type)
class _RE3D_API DyObject
	:public CObject
{
public:
	//virtual void setParams(const char *params);

	virtual void exec(const char *cmd, const char *args, void *param1 = nullptr, void *param2 = nullptr);

	//virtual DyObject* clone() =0 ;

public:
	static int   registerType(CreateObjectFuncT createFunc, const char *group, const char *type, int flag);

	static DyObject* createObject(const char *group, const char *type);

	template<typename _T>
	static _T* createObjectT(const char *group, const char *type)
	{
		return (_T*)createObject(group, type);
	}
};

#define DEFINE_RE3D_TYPE2(_class,typeName) public:\
	virtual void release() {delete this;}\
	static const char* getTypeName() {return typeName;}

#define DEFINE_RE3D_TYPE(_class) DEFINE_RE3D_TYPE2(_class,#_class)


#define DEFINE_RE3D_GROUP2(_class,groupName) \
public:\
static std::shared_ptr<_class> create(const char *type = nullptr){\
	return std::shared_ptr<_class>(\
		createObjectT<_class>(groupName, type),\
		[](DyObject *ptr) {ptr->release(); }\
	);}\
static const char* getGroupName() {return groupName;}

#define DEFINE_RE3D_GROUP(_class) DEFINE_RE3D_GROUP2(_class,#_class)


#define REGISTER_RE3D_TYPE(_class, flag) namespace _Re3D_UNIQUE_NAME(_re3d_regtype){ static void* createObject(){return new _class;} \
 static int _auto_register_re3dobjs=re3d::DyObject::registerType(createObject, _class::getGroupName(), _class::getTypeName(), flag);}

//#define REGISTER_RE3D_TYPE(objClass, flag) REGISTER_RE3D_TYPE2(objClass, #objClass, flag)

//===============================================================

class Variable
{
public:
	class Data
		:public CObject
	{
	public:
		template<typename _DataT>
		_DataT* getData()
		{
			if (this->getTypeID() != typeIDFromDataT<_DataT>())
			{
				throw std::invalid_argument("invalid variable type");
			}

			return (_DataT*)this->getData();
		}

		virtual void* getData() = 0;

		template<typename _DataT>
		static size_t  typeIDFromDataT();
	};

	template<typename _DataT>
	class Holder
		:public Data
	{
		_DataT _data;
	public:
		Holder() = default;

		Holder(const _DataT &data)
			:_data(data)
		{
		}

		virtual void* getData() {
			return &_data;
		}
	};
private:
	typedef std::shared_ptr<Data>  _DataPtr;


	static _DataPtr _makePtr(Data *ptr) {
		return _DataPtr(
			ptr, [](Data *_ptr) {_ptr->release(); }
		);
	}

	_DataPtr  _ptr=nullptr;
public:
	Variable() = default;

	template<typename _DataT>
	Variable(const _DataT &val)
	{
		this->make<_DataT>() = val;
	}

	bool empty() const
	{
		return !_ptr;
	}
	//raise exception if failed
	template<typename _DataT>
	_DataT& get()
	{
		return *verify(_ptr)->getData<_DataT>();
	}
	/*template<typename _DataT>
	std::vector<_DataT>& getVector()
	{
		return this->get<std::vector<_DataT>>();
	}
*/
	//create the object as necessary
	template<typename _DataT>
	_DataT& make()
	{
		if (!_ptr || _ptr->getTypeID() != Data::typeIDFromDataT<_DataT>())
		{
			_ptr = _makePtr(new Holder<_DataT>());
		}

		return get<_DataT>();
	}

	//use make() to explicitly specify the data type
	template<typename _DataT>
	Variable& operator=(const _DataT &val)
	{
		this->make<_DataT>() = val;
		return *this;
	}
};

template<typename _DataT>
inline size_t Variable::Data::typeIDFromDataT() {
	Variable::Holder<_DataT> var;
	return typeid(var).hash_code();
}



/*Managing a set of variables that can be set and get with a string name
*/
class _RE3D_API ObjectSet
	:public CObject
{
	class CImpl;
	std::shared_ptr<CImpl>  iptr;
public:
	ObjectSet();

	ObjectSet(const ObjectSet&) = delete;
	ObjectSet& operator=(const ObjectSet&) = delete;

	Variable*  query(const char *name);

	Variable&  get(const char *name);

	Variable&  make(const char *name);

	//return false if not found
	bool    remove(const char *name);

	template<typename _T>
	_T* getManaged() {
		//return reinterpret_cast<_T*>(this->getObject(_T::getTypeName()));
		return dynamic_cast<_T*>(this->get(_T::getTypeName()).get<CObject*>());
	}

	template<typename _DataT>
	_DataT& get(const char *name)
	{
		return get(name).get<_DataT>();
	}
	template<typename _DataT>
	_DataT& make(const char *name)
	{
		return make(name).make<_DataT>();
	}

	Variable& operator[](const char *name) {
		return make(name);
	}

protected:
	//call by getObject to create new object
	virtual CObject* _createObject(const char *name);
};


class ModelSet;

class _RE3D_API ModelInfos
{
public:
	std::string  name;
	std::string  owner;
	std::string  versionCode;
	std::string  uniqueID;

	std::string  modelFile;
	std::string  dataFile;
};

class _RE3D_API Model
	:public ObjectSet
{
public:
	class _RE3D_API ManagedObject
		:public DyObject
	{
	public:
		DEFINE_RE3D_GROUP2(ManagedObject,"ModelObject")
	public:
		virtual void onCreate(Model &model) = 0;
	};
	
private:
	class CImpl;
	CImpl  *iptr;
public:
	Model(/*ModelSet *modelSet*/);

	~Model();
protected:
	virtual CObject* _createObject(const char *name);
public:
	Model(const Model &) = delete;
	Model& operator=(const Model &) = delete;

	void set(const ModelInfos &infos);

	CVRModel& get3DModel();

	//const std::string& get3DModelFile();

	StreamSet& getData(bool createIfNotExist = true);

	const ModelInfos& getInfos();
};

typedef std::shared_ptr<Model>  ModelPtr;

//managing all models of the app.
class _RE3D_API ModelFactory
	:public CObject
{
	class CImpl;
	CImpl *iptr;
public:
	ModelFactory();

	~ModelFactory();

	ModelFactory(const ModelFactory &) = delete;
	ModelFactory& operator=(const ModelFactory&) = delete;

	ModelPtr  getModel(const ModelInfos &infos);
};


class _RE3D_API ModelSetInfos
{
public:
	std::string              name;
	std::string              owner;
	std::string              versionCode;
	std::string              uniqueID;

	std::string              dataFile;
	std::vector<ModelInfos>  models;
};

class _RE3D_API ModelSet
	:public ObjectSet
{
public:
	class _RE3D_API ManagedObject
		:public DyObject
	{
	public:
		DEFINE_RE3D_GROUP2(ManagedObject,"ModelSetObject")
	public:
		virtual void onCreate(ModelSet &modelSet) = 0;
	};

private:
	class CImpl;
	CImpl *iptr;
protected:
	//call by getObject to create new object
	virtual CObject* _createObject(const char *name);
public:
	ModelSet();

	~ModelSet();

	ModelSet(const ModelSet&) =delete;
	ModelSet& operator=(const ModelSet&) = delete;

	void set(const ModelSetInfos &infos);

	void set(const std::vector<ModelInfos> &models, const std::string &owner = "", const std::string &name = "");

	int size();

	//return nullptr if not found
	// idx : index of the model, in the range [0, nModels()-1]
	ModelPtr getModel(int idx);
	ModelPtr getModel(const std::string &name);
	
	int    getModelIndex(Model *p);
	int    getModelIndex(const ModelPtr &p)
	{
		return getModelIndex(p.get());
	}
	int    getModelIndex(const std::string &name);
	
	//throw exception if not found
	//Model& operator[](int idx);
	//Model& operator[](const std::string &name);

	const ModelSetInfos& getInfos();

	StreamSet& getData(bool createIfNotExist = true);
};


_RE3D_API std::vector<ModelInfos>  modelInfosFromFiles(const std::vector<std::string>  &modelFiles, const std::string &owner="");

inline std::vector<ModelInfos>  modelInfosFromSingleFile(const std::string  &modelFile, const std::string &owner = "")
{
	return modelInfosFromFiles({ modelFile }, owner);
}

_RE3D_API std::vector<ModelInfos>  modelInfosFromDir(const std::string &modelDir, const std::string &owner = "", bool searchRecursive = false, const std::string &filter = ".ply;.3ds;.obj;.stl");

_RE3D_API std::vector<ModelInfos>  modelInfosFromListFile(const std::string &listFile, const std::string &owner = "");

class _RE3D_API App
	:public ObjectSet
{
public:
	class _RE3D_API ManagedObject
		:public DyObject
	{
	public:
		DEFINE_RE3D_GROUP2(ManagedObject, "AppObject")
	public:
		virtual void onCreate(App &app) = 0;
	};
protected:
	virtual CObject* _createObject(const char *name);
private:
	class CImpl;
	CImpl *iptr;

	App();

	~App();
public:
	ModelFactory  modelFactory;
public:
	//get the singel app. object
	static App* instance();

	void setTempDir(const std::string &dir);

	const std::string& getTempDir();

	std::string  getOwnerDataDir(const std::string &owner);

	ModelInfos  configModel(const std::string &modelFile, const std::string &owner="", const std::string &name="");

	ModelSetInfos configModelSet(const std::vector<ModelInfos> &models, const std::string &owner="", const std::string &name="");

	ff::ArgSet& args();
	void setArgs(const char *args);
};

inline App* app()
{
	return App::instance();
}

//=========================================================
//
//class _RE3D_API MeshInfo
//	:public Model::ManagedObject
//{
//	DEFINE_RE3D_TYPE(MeshInfo)
//public:
//	int        version;
//
//	cv::Vec3f  center;
//	cv::Vec3f  bbMin, bbMax;
//
//	cv::Matx44f  R0;  //rotate the model to a standard pose, defaultly computed with PCA
//	cv::Vec3f    bbSizeR0; //size of the bounding box after rotated with R0
//
//public:
//	virtual void onCreate(Model &model);
//
//	cv::Matx44f getUnitize() const;
//
//	void getFrameR0(cv::Point3f oxyz[4]);
//};

class _RE3D_API PoseBase
{
public:
	float  score=0.f;
};

class _RE3D_API RigidPose
	:public PoseBase
{
public:
	cv::Matx33f R;
	cv::Vec3f   t;
public:
	RigidPose() = default;

	RigidPose(const cv::Matx33f &_R, const cv::Vec3f &_t)
		:R(_R),t(_t)
	{}
	RigidPose(const cv::Vec3f &rvec, const cv::Vec3f &tvec);

	RigidPose(const cv::Mat &R_or_rvec, const cv::Mat &tvec);

	RigidPose(const cv::Vec6f& rtVec);

	cv::Vec6f getVec6() const;

	cv::Vec3f getRvec() const;
		

public:
	friend RigidPose operator*(const RigidPose &pose, float scale)
	{
		return RigidPose(pose.R, pose.t*scale);
	}
	friend RigidPose& operator*=(RigidPose &pose, float scale)
	{
		pose.t *= scale;
		return pose;
	}
};


class _RE3D_API ImageObject
	//:public ObjectSet
{
public:
	int		modelIndex; //index in the model-set
	
	float   score;

	cv::Rect  roi;
	
	cv::Mat   mask;
	
	Variable  pose;

	//reserved
	int           ival;
	cv::Mat       mval;
};


class _RE3D_API FrameData
	:public ObjectSet
{
public:
	class _RE3D_API ManagedObject
		:public DyObject
	{
	public:
		DEFINE_RE3D_GROUP2(ManagedObject,"FrameDataObject")
	};
protected:
	virtual CObject* _createObject(const char *name);
public:
	int          frameID = -1;
	cv::Matx33f  cameraK;

	std::vector<ImageObject>  objs;
};


class _RE3D_API FrameProc
	:public DyObject
{
	DEFINE_RE3D_GROUP(FrameProc)
public:
	virtual void init(ModelSet *models, ff::ArgSet *args = nullptr) = 0;

	virtual int pro(const cv::Mat &img, FrameData &fd, ff::ArgSet *args = nullptr) = 0;
};


_RE3D_API void* loadModule(const std::string &libFile);


_RE3D_END
