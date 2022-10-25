
#include"cvrender.h"
#include"opencv2/highgui.hpp"
#include"CVX/gui.h"
using namespace cv;
#include"BFCS/log.cpp"
 
int main()
{
    cvrInit("-display :0.0");

    std::string dataDir="/fan/dev/prj-c1/1100-Re3DX/TestRe3DX/3ds/";
    std::string modelName="cup1";

#if 1
    //namedWindow("model");
    //std::string modelFile=dataDir+"/"+modelName+".3ds";
    //std::string modelFile="/fan/store/datasets/BOP/lmo_models/models/obj_000008.ply";
    //std::string modelFile="/fan/store/datasets/BOP/ycbv_models/models_fine/obj_000005.ply";
    //std::string modelFile="/fan/store/datasets/BOP/itodd_models/models/obj_000009.ply";
    //std::string modelFile="/fan/store/datasets/BOP/tyol_models/models/obj_000005.ply";

    //std::string modelFile="/fan/SDUicloudCache/re3d/test/3d/obj_01.ply";
    std::string modelFile="/fan/SDUicloudCache/re3d/test/3d/cat.obj";

    CVRModel model(modelFile);

    mdshow("model",model);
    cvxWaitKey(); 
    //cvrWaitFinish();
    //cvrUninit();

    return 0;

    Mat1b img(500,500);
    img=255;
   // imshowx("img",img);

    CVRender render(model);
    CVRMats mats(model,Size(800,800));
    CVRResult r=render.exec(mats,Size(800,800));

    mdshow("model",model);
   //imshow("imgx",r.img);

    cvxWaitKey();
#else
    Mat1b img(500,500);
    img=255;
    imshowx("img",img);
    cvxWaitKey();
#endif
    return 0;
}

int main2()
{
    cvrInit("-display :0.0");

    std::string dataDir="/fan/local/Re3D/models/";
    std::string modelName="bottle2";

    //std::string modelFile=dataDir+modelName+"/"+modelName+".3ds";
    std::string modelFile="/fan/SDUicloudCache/re3d/test/3d/obj_01.ply";
    CVRModel model(modelFile);

    CVRender render(model);
    Size viewSize(1200, 600);

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

#include<EGL/egl.h>
#include"BFC/log.h"
using namespace ff;

int main3()
{
    printf("#001\n");
    cvrInit("-display :0.0");
    //cvrInit();

    printf("#002\n");
   // return 0;
    std::string modelFile="./obj_01.ply";
    //std::string modelFile="/fan/dev/prj-c1/1100-Re3DX/TestRe3DX/3ds/bottle2.3ds";

    CVRModel model(modelFile);
    CVRender render(model);
    const int W=1200;
    CVRMats mats(model,Size(W,W));
    
    CVRResult r=render.exec(mats,Size(500,500));

    double etbeg=loget();
    for(int i=0; i<10; ++i)
        r=render.exec(mats,Size(W,W));
    printf("time=%.4f\n",(loget()-etbeg)/10);

    imwrite("./dimg.jpg",r.img);
    
    return 0;
}

