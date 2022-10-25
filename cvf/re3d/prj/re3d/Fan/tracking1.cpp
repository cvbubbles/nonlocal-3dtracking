#include"Base/cmds.h"

//#include"matchLF1a.h"
#include"tracking1a.h"

_CMDI_BEG

class Tracking1
	:public ICommand
{
public:
	virtual void exec(std::string dataDir, std::string args)
	{
		std::string modelName = "box1";
		std::string tempDir = "../datax/temp/";

		cv::VideoCapture cap;
		cap.open(dataDir + modelName+".mp4");
		//cap.set(CAP_PROP_POS_FRAMES, 180);

		Tracker tracker;
		tracker.load(dataDir + "/3d/" + modelName + ".3ds", tempDir + modelName + ".md.akaze");

		Mat img;
		int iframe = 0;
		while (cap.read(img))
		{
			printf("pos=%d\n", iframe++);
			resize(img, img, img.size() / 2);
			flip(img, img, -1);

			imshow("img", img);

			tracker.track(img);

			waitKey(10);
		}
	}

	virtual Object* clone()
	{
		return new Tracking1(*this);
	}
};

REGISTER_CLASS(Tracking1)

_CMDI_END
