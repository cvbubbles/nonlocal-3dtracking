
#include"appstd.h"

_CMDI_BEG


void show_models_drag_drop()
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
				CVRModel model(file);

				//model.saveAs(ff::ReplacePathElem(file, "obj", ff::RPE_FILE_EXTENTION));

				mdshow(file, model, Size(500, 500));
			}
		}
	}, "showModels");

	cv::cvxWaitKey();
}

CMD_BEG()
CMD0("tools.render.show_models_drag_drop", show_models_drag_drop)
CMD_END()

void show_model_file()
{
	//std::string file = R"(F:\dev\prj-c1\1100-Re3DX\TestRe3DX\3ds\plane.3ds)";

	//std::string file = R"(F:\SDUicloudCache\re3d\3ds-model\plane\plane.3ds)";
	
	std::string file = R"(F:\T\adizero_F50_TRX_FG_LEA\meshes\model.obj)";

	CVRModel model;

#if 0
	model.load(file, 3, "-extFile -");
	Matx44f m = model.calcStdPose();
	m = m*cvrm::rotate(CV_PI, Vec3f(1, 0, 0));

	model.setTransformation(m);
	std::string xfile = ff::ReplacePathElem(file, "yml", ff::RPE_FILE_EXTENTION);
	{
		cv::FileStorage xfs(xfile, FileStorage::WRITE);
		xfs << "pose0" << cv::Mat(m);
	}
#else
	model.load(file);
#endif

	mdshow("model", model);
	cv::cvxWaitKey();
}
CMD_BEG()
CMD0("tools.render.show_model_file", show_model_file)
CMD_END()


void renderModelToVideo(const std::string &modelFile, const std::string &vidFile, const std::string &imageFile)
{
	CVRModel model(modelFile);
	CVRender render(model);
	Size viewSize(800, 800);
	render.setBgColor(1, 1, 1);

	CVRMats matsInit(model, viewSize);

	cv::VideoWriter vw;
	vw.open(vidFile, CV_FOURCC('M', 'J', 'P', 'G'), 15, viewSize);

	matsInit.mModel = cvrm::rotate(-CV_PI / 2, Vec3f(1, 0, 0));

	float angle = 0;
	float delta = CV_PI * 2 / 100;
	bool isFirst = true;
	while (true)
	{
		angle += delta;

		CVRMats mats(matsInit);

		//rotate the object
		mats.mModel = mats.mModel * cvrm::rotate(angle, Vec3f(0, 1, 0));

		//rotate 90 degrees around y-axis
		//if (true)
		//	mats.mModel = cvrm::rotate(CV_PI / 2, Vec3f(1, 0, 0))*mats.mModel;

		//CVRResult result = render.exec(mats, viewSize, 3, CVRM_DEFAULT&~CVRM_ENABLE_TEXTURE);
		CVRResult result = render.exec(mats, viewSize, CVRM_IMAGE);
		flip(result.img, result.img, -1);

		imshow("img", result.img);
		if (angle>delta + 1e-6f) //ignore the first frame
		{
			if (isFirst && !imageFile.empty())
			{
				imwrite(imageFile, result.img);
				isFirst = false;
			}
			vw.write(result.img);
		}

		waitKey(1);
		//if (waitKey(1) == KEY_ESCAPE)
		if (angle >= CV_PI * 2)
			break;
}
}

void renderDirModelsToVideo(const std::string &dir)
{
	//std::string dataDir = D_DATA + "/re3d/3ds-models/";

	std::vector<std::string> subDirs;
	ff::listSubDirectories(dir, subDirs, false, false);

	for (auto &d : subDirs)
	{
		std::string name = ff::GetFileName(d, false);
		if (ff::IsDirChar(name.back()))
			name.pop_back();

		std::string file = dir + d + "/" + name + ".obj";
		if (ff::pathExist(file))
		{
			printf("%s\n", file.c_str());
			std::string vidFile = dir + d + "/" + name + ".avi";
			std::string imageFile = dir + "/" + name + ".png";
			renderModelToVideo(file, vidFile, imageFile);
		}
		//break;
	}
}

void on_renderToVideo()
{
	//renderDirModelsToVideo(D_DATA + "/re3d/3ds-model/");
	//renderDirModelsToVideo(D_DATA + "/re3d/models-618/");
	renderDirModelsToVideo(D_DATA + "/re3d/test/");
}

CMD_BEG()
CMD("tools.render.render_model_as_video", on_renderToVideo, "render 3d models as video files", "", "")
CMD_END()


void set_model_pose()
{
	std::string dir = R"(F:\SDUicloudCache\re3d\scan\models-618\)";
	std::string ddir = dir + "../models-618-2/";

	//dir = ddir;

	std::vector<string> subDirs;
	ff::listSubDirectories(dir, subDirs);

	//for (auto name : subDirs)
	{
		/*if (name != "bottle4\\")
			continue;*/

		//printf("%s\n", name.c_str());
		//auto modelFile = dir + name + "\\" + ff::GetFileName(name) + ".3ds";

		std::string modelFile = R"(F:\Test1\car\models\car.3ds)";
		std::string name = "car";

		CVRModel model;
		model.load(modelFile,0);
		auto mT=model.estimatePose0();
		model.transform(mT);

		auto center = model.getCenter();
		model.transform(cvrm::translate(-center[0], -center[1], -center[2]));

		auto outDir = ddir + ff::GetFileName(name)+"/";
		if (!ff::pathExist(outDir))
			ff::makeDirectory(outDir);
		auto outFile=outDir+ ff::GetFileName(name) + ".3ds";
		model.saveAs(outFile);

		//break;
		mdshow("model", model);
		cvxWaitKey();
	}
}

CMD_BEG()
CMD0("tools.render.set_model_pose", set_model_pose)
CMD_END()

_CMDI_END



