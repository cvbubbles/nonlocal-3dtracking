
#include"Base/cmds.h"

_CMDI_BEG

class ShowModels
	:public ICommand
{  
public: 
	virtual void exec(std::string dataDir, std::string args)
	{
		printf("Please drag-and-drop 3D model files to the main window.\n");

		Mat3b img = Mat3b::zeros(300, 300);
		auto wnd = imshowx("main", img);
		wnd->setEventHandler([](int code, int param1, int param2, CVEventData data) {
			if (code == cv::EVENT_DRAGDROP)
			{
				auto vfiles = (std::vector<std::string>*)data.ptr;
				for (auto &file : *vfiles)
				{
					mdshow(file, CVRModel(file), Size(500, 500));
				}
			}
		},"showModels");

		cv::cvxWaitKey();
	}
	virtual Object* clone()
	{
		return new ShowModels(*this);
	}
};

REGISTER_CLASS(ShowModels)

_CMDI_END

