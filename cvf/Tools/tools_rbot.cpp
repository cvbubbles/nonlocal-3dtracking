
#include"appstd.h"
#include<fstream>
#include<iostream>
using namespace std;

_CMDI_BEG

std::vector<string> loadImageList(const std::string &dir, const std::string &sceneType)
{
	std::vector<string> files;
	ff::listFiles(dir, files, false);

	auto isTheScene = [&sceneType](const std::string &v) {
		FFAssert(v[1] == '_');
		return strncmp(sceneType.c_str(), v.c_str() + 2, (int)sceneType.size())==0;
	};

	std::vector<string> dfiles;
	std::copy_if(files.begin(), files.end(), std::back_inserter(dfiles), isTheScene);
	std::sort(dfiles.begin(), dfiles.end());
	return dfiles;
}

struct DPose
{
	Matx33f R;
	Vec3f   t;
};

std::vector<DPose> loadGT(const std::string &file)
{
	std::ifstream is(file.c_str());
	if (!is)
		throw file;

	DPose pose;
	static_assert(sizeof(pose) == sizeof(float) * 12,"");

	char buf[256];
	is.getline(buf, sizeof(buf)); //skip the first line

	std::vector<DPose>  v;
	while (is)
	{
		for (int i = 0; i < 12; ++i)
			is >> ((float*)(&pose))[i];
		if (is)
			v.push_back(pose);
	}
	return v;
}

void show_rbot_gt()
{
	std::string ddir = R"(F:\store\datasets\RBOT_dataset\)";
	std::string modelName = "cat";
	std::string sceneType = "regular";

	std::string modelDir = ddir + modelName + "/";
	std::string imgDir = modelDir + "frames/";

	auto imgFiles = loadImageList(imgDir, sceneType);
	auto gt = loadGT(ddir + "poses_first.txt");
	FFAssert(imgFiles.size() == gt.size());

	CVRModel model(modelDir + modelName + ".obj");
	CVRender render(model);

	Matx33f K = {
		650.048f, 0, 324.328f,
		0,     647.183f, 257.323f,
		0, 0, 1
	};

	for(size_t i=0; i<imgFiles.size(); ++i)
	{
		auto &f = imgFiles[i];
		auto img = imread(imgDir + f);

		render.setBgImage(img);
		CVRMats mats;
		mats.mModel = cvrm::fromR33T(gt[i].R, gt[i].t);
		mats.mProjection = cvrm::fromK(K,img.size(),1,1000);
		auto rr = render.exec(mats, img.size());

		imshow("img", img);
		imshow("gt", rr.img);

		cv::waitKey(50);
	}
}

CMD_BEG()
CMD0("tools.rbot.show_rbot_gt", show_rbot_gt)
CMD_END()



_CMDI_END

