
#include"object.h"
#include"BFC/err.h"


void Object::setArgs(ff::ArgSet &args)
{
}

Object::~Object()
{}

Factory::Factory()
{
}
Factory* Factory::instance() 
{
	static Factory f;
	return &f;
}

void Factory::registerType(Object *obj, const std::string &name)
{
	obj->name = name;
	_objList.push_back(ObjectType(-1, obj));
}

Object* Factory::create(const std::string &name)
{
	Object *obj=nullptr;
	for (auto &v : _objList)
		if (stricmp(v.obj->name.c_str(),name.c_str()) == 0)
		{
			obj = v.obj;
			break;
		}
	if (obj)
		obj = obj->clone();
	return obj;
}
