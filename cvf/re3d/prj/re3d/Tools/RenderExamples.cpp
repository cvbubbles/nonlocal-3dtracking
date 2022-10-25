
#include"Base/cmds.h"
#include"BFC/portable.h"

_CMDI_BEG

class RenderExamples
	:public ICommand
{
public:
	//load and show model
	virtual void exec1(std::string dataDir, std::string args)
	{
		mdshow("model1", CVRModel(dataDir + "/3d/box1.3ds"));

		CVRModel model2(dataDir + "/3d/obj_01.ply");
		cv::Mat bg = imread(dataDir + "/bg/bg1.jpg");
		mdshow("model2", model2, bg.size(), CVRM_DEFAULT, bg);

		bg = imread(dataDir + "/bg/bg2.jpg");
		mdshow("model3", CVRModel(dataDir + "/3d/box2.3ds"),bg.size(),CVRM_DEFAULT,bg);

		cvxWaitKey();
	}
	//render to image
	virtual void exec2(std::string dataDir, std::string args)
	{
		CVRModel model(dataDir + "/3d/box2.3ds");

		CVRender render(model);
		Size viewSize(800, 800);
		
		CVRMats mats(model, viewSize); //init OpenGL matrixs to show the model in the default view
		
		CVRResult result = render.exec(mats, viewSize);

		imshow("img", result.img);
		imshow("depth", result.depth);

		Point3f pt=result.unproject(400, 400);
		printf("unproject pt=(%.2f,%.2f,%.2f)\n", pt.x, pt.y, pt.z);

		Mat bg = imread(dataDir + "/bg/bg1.jpg");
		render.setBgImage(bg);
		mats = CVRMats(model, bg.size());
		result = render.exec(mats, bg.size());

		imshow("imgInBG", result.img);

		cvxWaitKey();
	}
	//set GL matrix
	virtual void exec(std::string dataDir, std::string args)
	{
		CVRModel model(dataDir + "/3d/box2.3ds");
		CVRender render(model);
		Size viewSize(800, 800);

		CVRMats matsInit(model, viewSize); 

		float angle = 0;
		float dist = 0, delta=0.1;
		while (true)
		{
			angle += 0.05;
			dist += delta;
			if (dist < 0 || dist>20)
				delta = -delta;

			CVRMats mats(matsInit);
			
			//rotate the object
			mats.mModel = mats.mModel * cvrm::rotate(angle, Vec3f(1, 0, 0));
			
			//move the camera
			mats.mView = mats.mView * cvrm::translate(0, 0, -dist);

			CVRResult result = render.exec(mats, viewSize);
			imshow("img", result.img);

			if (waitKey(30) == KEY_ESCAPE)
				break;
		}
	}
	//samples views on the sphere
	virtual void exec4(std::string dataDir, std::string args)
	{
		int nSamples = 1000;

		std::vector<Vec3f> vecs;
		cvrm::sampleSphere(vecs, nSamples);

		CVRModel model(dataDir+"/3d/car.3ds");
		CVRender render(model);

		Size viewSize(500, 500);
		CVRMats objMats(model, viewSize);

		for(auto v : vecs)
		{
			printf("(%.2f,%.2f,%.2f)\n", v[0], v[1], v[2]);
			objMats.mModel = cvrm::rotate(v, Vec3f(0, 0, 1));
			CVRResult res = render.exec(objMats, viewSize);
			imshow("img", res.img);
			cv::waitKey(1000);
		}
	}
	virtual Object* clone()
	{
		return new RenderExamples(*this);
	}
};

REGISTER_CLASS(RenderExamples)

_CMDI_END

