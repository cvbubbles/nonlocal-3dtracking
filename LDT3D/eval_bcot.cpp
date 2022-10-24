#pragma once

#include"stdafx.h"
#include"evaluator.h"

#include"tracker_ldt3d.h"

namespace vx = tracker_v1;

int b_fi = 0;

_STATIC_BEG
float getModelDiameter(const CVRModel& model)
{
	Vec3f bbSize = model.getSizeBB();
	return max(max(bbSize[0], bbSize[1]), bbSize[2]);
}
std::string D_BCOT = "D:/datasets/BCOT/BCOT Benchmark/";

static void test_bcot_all()
{
	std::string bcot_models_path = D_BCOT + "models/";
	
	vector<std::string> all_scenes;
	ff::listSubDirectories(D_BCOT, all_scenes,false,false);
	
	struct DModel
	{
		std::string sceneName;
		std::string name;
	};
	uint totalTime = 0, totalFrames = 0;
	int anLost55 = 0, anLost22 = 0;
	int anLost_add002d = 0, anLost_add005d = 0, anLost_add01d = 0;
	int anLost5t = 0, anLost5r = 0, anLost2t = 0, anLost2r = 0;
	int anTotal = 0;
	for (int i = 0; i < all_scenes.size(); i++)
	{
		std::vector<DModel>  accModels;
		uint totalSceneTime = 0, totalSceneFrames = 0;
		if (all_scenes[i] == "models\\") continue;
		std::string scene_path = D_BCOT + all_scenes[i] + "/";
		std::vector<std::string> body_names;
		ff::listSubDirectories(scene_path, body_names, false, false);
		cv::Matx33f K = read_BCOT_K(scene_path + "K.txt");
		
		for (auto& modelName : body_names)
		{
			modelName = modelName.substr(0, modelName.length() - 1);
			std::vector<Pose> gtPoses = read_gt_poses(scene_path + modelName + "/pose.txt",true);
			vx::Tracker tracker;

			CVRModel model(bcot_models_path + modelName + ".obj");
			float model_diameter = getModelDiameter(model);
			
			vector<cv::Vec3f> modelPts = model.getVertices();// origin pts of obj file
			
			std::string imgDir = scene_path + modelName + "/";
			auto loadImage = [&imgDir](int fi) {
				auto file = imgDir + ff::StrFormat("%04d.png", fi);
				return cv::imread(file, cv::IMREAD_COLOR);
			};

			tracker.loadModel(model.getFile(), "");

			int start = 0;
			Pose pose = gtPoses[start];
			tracker.reset(loadImage(start), pose, K);
			printf("\n");
			
			const int S = 1;

			for (int fi = start + S; fi <= gtPoses.size()-1; fi += S)
			{
				b_fi = fi;
				cv::Mat3b img = loadImage(fi);

				time_t beg = clock();

				tracker.startUpdate(img, Pose());
				tracker.update(pose);
				tracker.endUpdate(pose);
				
				float timec = uint(clock() - beg);
				totalTime += timec;
				++totalFrames;
				totalSceneTime += timec;
				++totalSceneFrames;
				printf("fi=%d, time=%dms     \r", fi, int(clock() - beg));
				if (isLost(pose, gtPoses[fi],2.0f,false,false))
				{
					++anLost22;
				}
				if (isLost(pose, gtPoses[fi],2.0f,false,true))
				{
					++anLost2t;
				}
				if (isLost(pose, gtPoses[fi],2.0f,true,false))
				{
					++anLost2r;
				}
				if (isLost(pose, gtPoses[fi],5.0f,false,true))
				{
					++anLost5t;
				}
				if (isLost(pose, gtPoses[fi],5.0f,true,false))
				{
					++anLost5r;
				}
				float add = calADD(pose, gtPoses[fi], modelPts);
				if (add > 0.02f * model_diameter)
				{
					++anLost_add002d;
				}
				if (add > 0.05f * model_diameter)
				{
					++anLost_add005d;
				}
				if (add > 0.1f * model_diameter)
				{
					++anLost_add01d;
				}
				if (isLost(pose, gtPoses[fi],5.0f,false,false))
				{

					printf("E%d:", fi);
					printLost(pose.R, pose.t, gtPoses[fi].R, gtPoses[fi].t);

					pose = gtPoses[fi];
					tracker.reset(img, pose, K);
					++anLost55;
				}
				++anTotal;
			}

		}

		
		
	}
	//statistics of all scenes
	printf("all scenes:\n");
	
	printf("acc_add002d=%.2f\n", 100.f * (anTotal - anLost_add002d) / anTotal);
	printf("acc_add005d=%.2f\n", 100.f * (anTotal - anLost_add005d) / anTotal);
	printf("acc_add01d=%.2f\n", 100.f * (anTotal - anLost_add01d) / anTotal);

	printf("acc55=%.2f\n", 100.f * (anTotal - anLost55) / anTotal);
	printf("acc_add5r=%.2f\n", 100.f * (anTotal - anLost5r) / anTotal);
	printf("acc_add5t=%.2f\n", 100.f * (anTotal - anLost5t) / anTotal);

	printf("acc22=%.2f\n", 100.f * (anTotal - anLost22) / anTotal);
	printf("acc_add2r=%.2f\n", 100.f * (anTotal - anLost2r) / anTotal);
	printf("acc_add2t=%.2f\n", 100.f * (anTotal - anLost2t) / anTotal);

	printf("meanTime=%.2f\n", float(totalTime) / totalFrames);
	
}
CMD_BEG()
CMD0("trackers.test_on_bcot", test_bcot_all)
CMD_END()


_STATIC_END