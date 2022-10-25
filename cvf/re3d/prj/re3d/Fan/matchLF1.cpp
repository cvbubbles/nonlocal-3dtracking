#include"Base/cmds.h"

#include"matchLF1a.h"

_CMDI_BEG 

class MatchLF1 
	:public ICommand
{
public:
	virtual void exec(std::string dataDir, std::string args)
	{
		std::string outDir = "../datax/temp/";

		proModel(dataDir, outDir);
		//matchModel(dataDir, outDir);
	}

	virtual Object* clone()
	{ 
		return new MatchLF1(*this);
	}
};

REGISTER_CLASS(MatchLF1)

_CMDI_END
