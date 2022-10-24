#pragma once

#include"CVX/core.h"

cv::Mat1b vseg_get_trimap(const cv::Mat1b& mask, int maxFilterSize = 0);

cv::Mat1b vseg_global(const cv::Mat3b& ref, const cv::Mat1b& refTrimap, const cv::Mat3b& tar);

cv::Mat1b vseg_local(const cv::Mat3b& ref, const cv::Mat1b& refMask, const cv::Mat3b& tar, int localWindowSize=51);

void vseg_remove_unknown_regions(cv::Mat1b& prob, uchar T0=122, uchar T1=132, double rT=0.9);

void vseg_remove_small_regions(cv::Mat1b& prob, int minRegionSize=-1, uchar T=127);

cv::Mat1b vseg_patchmatch(const cv::Mat3b& ref, const cv::Mat1b& refMask, const cv::Mat3b& tar, int patchSize);


