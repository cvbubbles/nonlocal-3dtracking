
#include"stdafx.h"
#include"evaluator.h"

#include"tracker_ldt3d.h"

namespace vx = tracker_v1;

int g_fi = 0;

_STATIC_BEG

std::string  D_RBOT = "D:/datasets/RBOT/";

inline Mat1b renderObjMask(CVRModel &model, const Pose& pose, const Matx33f& K, Size dsize)
{
	CVRMats mats;
	mats.mModel = cvrm::fromR33T(pose.R, pose.t);
	mats.mProjection = cvrm::fromK(K,dsize,1,3000);

	CVRender render(model);

	CVRResult rr = render.exec(mats, dsize, CVRM_DEPTH);

	return getRenderMask(rr.depth);
}

static void on_trackers_test_accuracy()
{
	cv::Matx33f K = getRBOT_K();

	std::string dataPath = D_RBOT;
	std::vector<Pose> gtPoses = read_gt_poses(dataPath + "poses_first.txt");

	/*std::vector<std::string> body_names{
		"ape",  "bakingsoda", "benchviseblue", "broccolisoup", "cam",
		"can",  "cat",        "clown",         "cube",         "driller",
		"duck", "eggbox",     "glue",          "iron",         "koalacandy",
		"lamp", "phone",      "squirrel" };*/

	std::string modelName = "ape";

	CVRModel model(dataPath + modelName + "/" + modelName + ".obj");
	VisHandler visHdl(model, K);

	std::string imgDir = dataPath + modelName + "/frames/";
	auto loadImage = [&imgDir](int fi) {
		auto file = imgDir + ff::StrFormat("a_regular%04d.png", fi);
		//auto file = imgDir + ff::StrFormat("b_dynamiclight%04d.png", fi);
		//auto file = imgDir + ff::StrFormat("c_noisy%04d.png", fi);
		return cv::imread(file, cv::IMREAD_COLOR);
	};

	vx::Tracker tracker; 
	tracker.loadModel(model.getFile(),"");

	int start = 0;
	Pose pose = gtPoses[start];
	tracker.reset(loadImage(start), pose, K);

	int nLost = 0, nTotal = 0, totalTime=0;
	const int S = 1;
	for (int fi = start+S; fi <= 1000; fi += S)
	{
		g_fi = fi;

		cv::Mat3b img = loadImage(fi);

		time_t beg = clock();
		tracker.startUpdate(img, gtPoses[fi]);

		auto R0 = pose.R;
		tracker.update(pose);

		tracker.endUpdate(pose);
		totalTime += int(clock() - beg);
		printf("fi=%d, time=%dms     \r", fi, int(clock() - beg));

		if (isLost(pose, gtPoses[fi]))
		{
			printf("E%d",fi);
			printLost(pose.R, pose.t, gtPoses[fi].R, gtPoses[fi].t);
			printf("#%d",fi);
			printLost(gtPoses[fi - S].R, gtPoses[fi - S].t, gtPoses[fi].R, gtPoses[fi].t);
			printf("\n");

			pose = gtPoses[fi];
			tracker.reset(img, pose, K);
			++nLost;
		}
		++nTotal;

		if (cv::waitKey() == 'q')
			break;
	}
	printf("updatesPerIm=%.2f\n", float(vx::g_totalUpdates) / nTotal);
	printf("acc=%.2f  meanTime=%.2f   \n", 100.f * (nTotal - nLost) / nTotal, float(totalTime)/nTotal);
}

CMD_BEG()
CMD0("trackers.test_accuracy", on_trackers_test_accuracy)
CMD_END()



static void on_trackers_test_accuracy_all()
{
	cv::Matx33f K = getRBOT_K();

	std::string dataPath = D_RBOT;
	std::vector<Pose> gtPoses = read_gt_poses(dataPath + "poses_first.txt");
	
    std::vector<std::string> body_names{
		"ape",  "bakingsoda", "benchviseblue", "broccolisoup", "cam",
		"can",  "cat",        "clown",         "cube",         "driller",
		"duck", "eggbox",     "glue",          "iron",         "koalacandy",
		"lamp", "phone",      "squirrel" };
	
	struct DModel
	{
		std::string name;
		float       accuracy;
	};

	std::vector<DModel>  accModels;

	uint totalTime = 0, totalFrames = 0;

	for (auto& modelName : body_names)
	{
		vx::Tracker tracker; 

		CVRModel model(dataPath + modelName + "/" + modelName + ".obj");
		VisHandler visHdl(model, K);

		std::string imgDir = dataPath + modelName + "/frames/";
		auto loadImage = [&imgDir](int fi) {
			auto file = imgDir + ff::StrFormat("a_regular%04d.png", fi);
			//auto file = imgDir + ff::StrFormat("b_dynamiclight%04d.png", fi);
			//auto file = imgDir + ff::StrFormat("c_noisy%04d.png", fi);
			//auto file = imgDir + ff::StrFormat("d_occlusion%04d.png", fi);
			return cv::imread(file, cv::IMREAD_COLOR);
		};

		tracker.loadModel(model.getFile(), "");

		int start = 0;
		Pose pose = gtPoses[start];
		tracker.reset(loadImage(start), pose, K);

		printf("\n");
		int nLost = 0, nTotal = 0;
		const int S = 1;
		for (int fi = start + S; fi <= 1000; fi += S)
		{
			g_fi = fi; 


			cv::Mat3b img = loadImage(fi);

			time_t beg = clock();
			
			tracker.startUpdate(img, Pose());
			tracker.update(pose);
			tracker.endUpdate(pose);
			
			totalTime += uint(clock() - beg);
			++totalFrames;
			printf("fi=%d, time=%dms     \r", fi, int(clock() - beg));

			if (isLost(pose, gtPoses[fi]))
			{
				printf("E%d:", fi);
				printLost(pose.R, pose.t, gtPoses[fi].R, gtPoses[fi].t);

				pose = gtPoses[fi];
				tracker.reset(img, pose, K);
				++nLost;
			}
			
			++nTotal;
		}

		float acc = 100.f * (nTotal - nLost) / nTotal;
		printf("%s acc=%.2f   \n", modelName.c_str(), acc);
		accModels.push_back({ modelName, acc });
	}

	printf("\n");
	float mean = 0;
	for (auto& v : accModels)
	{
		printf("%s = %.2f\n", v.name.c_str(), v.accuracy);
		mean += v.accuracy;
	}
	mean /= accModels.size();
	printf("updatesPerIm=%.2f\n", float(vx::g_totalUpdates) / totalFrames);
	printf("meanTime=%.2f\n", float(totalTime) / totalFrames);
	printf("mean = %.2f\n", mean);
}

CMD_BEG()
CMD0("trackers.test_accuracy_all", on_trackers_test_accuracy_all)
CMD_END()

static void on_calc_displacements()
{
	std::string dataPath = D_RBOT;
	std::vector<Pose> gtPoses = read_gt_poses(dataPath + "poses_first.txt");


	int start = 0;

	double dRSum = 0, dtSum = 0, dRMax = 0, dtMax = 0;
	int n = 0;
	const int DF = 1;
	for (int fi = start + DF; fi <= 1000; fi += DF)
	{
		double dRx = get_errorR(gtPoses[fi].R, gtPoses[fi - DF].R);
		double dtx = get_errort(gtPoses[fi].t, gtPoses[fi - DF].t);
		dRSum += dRx;
		dtSum += dtx;
		if (dRx > dRMax)
			dRMax = dRx;
		if (dtx > dtMax)
			dtMax = dtx;
		++n;
	}
	printf("dR=%.2f, dt=%.2f\n", dRSum / n, dtSum / n);
	printf("max=%.2f, %.2f\n", dRMax, dtMax);
}

CMD_BEG()
CMD0("trackers.calc_displacements", on_calc_displacements)
CMD_END()
_STATIC_END


