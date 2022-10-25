
#pragma once

#ifdef _MSC_VER
#pragma warning(disable:4819)
#endif


#include<opencv2/core/mat.hpp>
#include<memory>
#include<list>
#include"BFC/argv.h"

class Object
{
public:
	std::string  name;
public:
	virtual void  setArgs(ff::ArgSet &args);

	//clone the object, used by factory to create objects from a registered object
	virtual Object* clone() = 0;

	virtual ~Object();
};


//factory to manage (register and create) Measure and Matcher types
class Factory
{
private:
	class ObjectType
	{
	public:
		int   group; //not used
		Object *obj;
	public:
		ObjectType(int _group=-1, Object *_obj=nullptr)
			:group(_group), obj(_obj)
		{}
	};

	std::list<ObjectType>  _objList;

	Factory();
public:
	//get the unique instance
	static Factory* instance();

	/*register a type
	@group : GROUP_MEASURE, GROUP_MATCHER, ...
	@obj   : an instance of the type
	*/
	void registerType(Object *obj, const std::string &name);

	Object* create(const std::string &name);

	template<typename _T>
	_T* createObject(const std::string &name)
	{
		return reinterpret_cast<_T*>(this->create(name));
	}
	template<typename _T>
	_T* createObject(const std::string &name, const std::string &argStr)
	{
		_T *obj = reinterpret_cast<_T*>(this->create(name));
		ff::ArgSet  args(argStr);
		obj->setArgs(args);
		return obj;
	}
};

class auto_register_obj_type
{
public:
	auto_register_obj_type(Object *obj, const std::string &name)
	{
		Factory::instance()->registerType(obj, name);
	}
};

#define REGISTER_CLASS_EX(cls, name) auto_register_obj_type auto_register_obj_type_##cls(new cls, name);

//register an object type to the factory
#define REGISTER_CLASS(className) REGISTER_CLASS_EX(className, #className)

#define _OBJ_BEG namespace {
#define _OBJ_END }
