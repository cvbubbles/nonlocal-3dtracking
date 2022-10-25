
#include<opencv2/highgui.hpp>
using namespace cv;

#include"CVX/gui.h"

#define USE_OPENCV3_LIB
#include"lib2.h"

int main()
{
	cv::namedWindow("img", WINDOW_NORMAL | CV_WINDOW_NORMAL);


	Mat img = imread("f:/dx/hsi/002.jpg");

	imshow("img", img);
	waitKeyEx();

	return 0;
}

