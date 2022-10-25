
#include"EDK/cmds.h"
#include"BFC/portable.h"
#include"BFC/stdf.h"
#include<fstream>
#include<time.h>
using namespace std;
using namespace ff;

//
//class DatasetRender
//{
//	std::vector<CVRModel>  models;
//	std::vector<CVRender>  renders;
//	
//public:
//	void loadModels(const std::vector<std::string> &files)
//	{
//		size_t size = files.size();
//		models.resize(size);
//		renders.resize(size);
//
//		for (size_t i = 0; i < size; ++i)
//		{
//			models[i].load(files[i]);
//			renders[i] = CVRender(models[i]);
//		}
//	}
//
//};
//
//
//#include"pybind11/pybind11.h"
//
//namespace py = pybind11;
//
//PYBIND11_MODULE(pyre3dx, m) {
//
//	//m.doc() = "re3d dataset render";
//
//	py::class_<DatasetRender>(m, "DatasetRender")
//		.def("loadModels", &DatasetRender::loadModels);
//
//}

