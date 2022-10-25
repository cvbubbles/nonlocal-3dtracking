
#include"nlohmann/json.hpp"

#include"appstd.h"
#include"BFC/netcall.h"
#include<fstream>

_CMDI_BEG

void on_det2d()
{
	//ff::NetcallServer serv("10.102.32.173", 8011);
	ff::NetcallServer serv("101.76.215.159", 8011);

#if 1
	cv::VideoCapture vid;
	vid.open("../data/det2d-5.avi");
	Mat img;
	while (vid.read(img))
	{
#else
	//std::string dir = R"(F:\store\datasets\BOP\ycbv_test_bop19\test\000048\rgb\)";
	//std::string dir = "f:/rgb/";
	std::string dir = R"(F:\re3d2a\eval\)";

	std::vector<string> files;
	ff::listFiles(dir, files);
	for(auto f : files)
	{
		Mat img = cv::imread(dir+f);
#endif
		Mat dimg = img.clone();
		//resize(dimg, dimg, dimg.size() / 2);
		//cvtColor(dimg, dimg , CV_BGR2RGB);

		ff::NetObjs objs = {
			{ "cmd","run" },
			{ "img",ff::ObjStream::fromImage(dimg,".png") }
		};

		try {
			ff::NetObjs dobjs = serv.call(objs);
			if (!dobjs)
				continue;

			auto labels = dobjs["labels"].get<std::vector<std::string>>();
			auto bboxes = dobjs["bboxes"].get<Mat1f>();
			auto scores = dobjs["scores"].get<std::vector<float>>();

			dimg = img.clone();
			for (int i = 0; i < (int)labels.size(); ++i)
			{
				if (scores[i] > 0.5f)
				{
					const float *bb = bboxes.ptr<float>(i);
					//int x = int(bb[0]);
					//cv::Rect bbox(int(bb[0]), int(bb[1]), int(bb[2]), int(bb[3]));
					cv::Rect bbox(bb[0], bb[1], bb[2] - bb[0], bb[3] - bb[1]);
					cv::rectangle(dimg, bbox, Scalar(0, 255, 255), 2);

					std::string text = ff::StrFormat("%s(%.2f)", labels[i].c_str(), scores[i]);
					cv::putText(dimg, text, Point(bbox.x, bbox.y), cv::FONT_HERSHEY_PLAIN, 2.0, Scalar(255, 0, 0), 2, CV_AA);
				}
				//break;
			}
			imshow("dimg", dimg);
			if (cv::waitKey(10) == 'q')
				break;
		}
		catch (const std::exception& ec)
		{
			printf("%s\n", ec.what());
		}
	}
	serv.sendExit();
}

CMD_BEG()
CMD0("tools.netcall.det2d", on_det2d)
CMD_END()

Mat1b getRenderMask(const Mat1f &depth, float eps = 1e-6f)
{
	Mat1b mask = Mat1b::zeros(depth.size());
	for_each_2(DWHN1(depth), DN1(mask), [eps](float d, uchar &m) {
		m = fabs(1.0f - d)<eps ? 0 : 255;
	});
	return mask;
}


Matx33f getK(float cx, float cy, float fx, float fy)
{
	Matx33f K = Matx33f::eye();

	K(0, 0) = fx;
	K(1, 1) = fy;
	K(0, 2) = cx;
	K(1, 2) = cy;
	return K;
}

struct CameraInfo
{
	Matx33f K;
	Size    imgSize;
};
CameraInfo getCameraInfo(const std::string &file)
{
	std::ifstream is(file);

	nlohmann::json  jf;
	is >> jf;

	CameraInfo ci;
	ci.K = getK(jf["cx"].get<float>(), jf["cy"].get<float>(), jf["fx"].get<float>(), jf["fy"].get<float>());
	ci.imgSize = Size(jf["width"].get<int>(), jf["height"].get<int>());
	return ci;
}


/*
调用cosypose服务器检测并显示结果

cosypose服务器端结果通过以下代码打包发回客户端：

def encodeResult(r):  //r是run_pred返回的结果
	x,y=r   //x是二维检测结果，y是三维检测结果
	labels = x.infos['label'].values
	nobjs,=labels.shape
	scores=x.infos['score'].values
	scores=scores.astype(np.float32)
	bboxes=x.bboxes.numpy()
	poses=y.poses.numpy()
	labels=[labels[i] for i in range(0,nobjs)]

	objs={'labels':labels, 'scores':scores, 'bboxes':bboxes, 'poses':poses}
	return netcall.encodeObjs(objs)

*/
void on_det3d()
{
	//ff::NetcallServer serv("10.102.32.173", 8011);
	ff::NetcallServer serv("101.76.215.159", 8011);

	std::map<std::string, CVRModel> models;
	std::map<std::string, CVRender> renders;

#if 0
	cv::VideoCapture vid;
	vid.open("../data/det2d-5.avi");
	Mat img;
	while (vid.read(img))
	{
		Matx33f K = cvrm::defaultK(img.size(), 1.2);
#else
	std::string modelDir = R"(f:/store/datasets/BOP/ycbv_models/models/)";
	std::string dir = R"(F:\store\datasets\BOP\ycbv_test_bop19\test\000054\rgb\)";
	auto ci = getCameraInfo(R"(f:/store/datasets/BOP/ycbv/camera_uw.json)");
	Matx33f K = ci.K;

	std::vector<string> files;
	ff::listFiles(dir, files);
	for (auto f : files)
	{
		Mat img = cv::imread(dir + f);
#endif
		Mat dimg;
		cvtColor(img, dimg, CV_BGR2RGB);

		try {
			
			ff::NetObjs objs = {
				{ "cmd","run" },
				{ "K",K },{ "img",ff::ObjStream::fromImage(dimg,".png") }
			};

			ff::NetObjs dobjs = serv.call(objs);
			if (!dobjs)
				continue;

			auto labels = dobjs["labels"].get<std::vector<std::string>>();
			auto bboxes = dobjs["bboxes"].get<Mat1f>();
			auto scores = dobjs["scores"].get<std::vector<float>>();
			auto poses = dobjs["poses"].get<Mat4f>();

			dimg = img.clone();
			for (int i = 0; i < (int)labels.size(); ++i)
			{
				if (scores[i] > 0.5f)
				{
					auto label = labels[i];
					if (renders.find(label) == renders.end())
					{
						std::string modelFile = modelDir + "/" + label + ".ply";
						models.emplace(label, CVRModel());
						models[label].load(modelFile);
						renders.emplace(label, CVRender(models[label]));
					}

					float *m = poses.ptr<float>(i);
					Matx44f rt = Mat(Mat(4, 4, CV_32FC1, m));
					Matx33f R;
					for (int j = 0; j < 3; ++j)
						for (int k = 0; k < 3; ++k)
							R(j, k) = rt(j, k);

					/*
					BOP数据集里cosypose输出的平移乘以了0.001，这里乘上1000复原
					不太清楚对我们自己的数据集是否需要这个变换
					*/
					Vec3f t;
					for (int j = 0; j < 3; ++j)
						t[j] = rt(j, 3)*1000;

					CVRMats mats;
					mats.mProjection = cvrm::fromK(K, dimg.size(), 1, 2000);
					mats.mModel = cvrm::fromR33T(R, t);

					auto rr = renders[label].exec(mats, dimg.size());
					Mat mask = getRenderMask(rr.depth);

					for_each_3(DWHNC(dimg), DNC(rr.img), DN1(Mat1b(mask)), [](uchar *a, const uchar *b, uchar m) {
						if (m)
							for (int i = 0; i < 3; ++i)
								a[i] = (a[i] + b[i]) / 2;
					});

					std::vector<std::vector<cv::Point>> contours;
					cv::findContours(mask, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);
					cv::drawContours(dimg, contours, -1, Scalar(0, 255, 255), 2);

					const float *bb = bboxes.ptr<float>(i);
					cv::Rect bbox(bb[0], bb[1], bb[2] - bb[0], bb[3] - bb[1]);
					//cv::rectangle(dimg, bbox, Scalar(0, 255, 255), 2);

					std::string text = ff::StrFormat("%s(%.2f)", labels[i].c_str(), scores[i]);
					cv::putText(dimg, text, Point(bbox.x, bbox.y), cv::FONT_HERSHEY_PLAIN, 2.0, Scalar(255, 0, 0), 2, CV_AA);
				}
			}
			imshow("dimg", dimg);
			if (cv::waitKey(0) == 'q')
				break;
		}
		catch (const std::exception& ec)
		{
			printf("%s\n", ec.what());
		}
	}
	serv.sendExit();
}

CMD_BEG()
CMD0("tools.netcall.det3d", on_det3d)
CMD_END()

_CMDI_END

