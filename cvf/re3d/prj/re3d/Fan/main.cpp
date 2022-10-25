
#include"Base/cmds.h"

int main()
{
	std::string dataDir = D_DATA + "/re3d/test/";

	auto fact = Factory::instance();

	//fact->createObject<ICommand>("MatchLF1")->exec(dataDir);
	fact->createObject<ICommand>("Tracking1")->exec(dataDir);


	return 0;
}

