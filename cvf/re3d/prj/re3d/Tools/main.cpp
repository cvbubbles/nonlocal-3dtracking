
#include"Base/cmds.h"

int main()
{
	std::string dataDir = D_DATA+"/re3d/test/";

	auto fact = Factory::instance();

	//fact->createObject<ICommand>("ShowModels")->exec(dataDir);
	fact->createObject<ICommand>("RenderExamples")->exec(dataDir);
	

	return 0;
}

