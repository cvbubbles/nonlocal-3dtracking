
#pragma once

#include"CVX/def.h"
#include"opencv2/core.hpp"

_CVX_BEG

int readGIF(const std::string &file, std::vector<Mat> &frames, bool readAlpha=false, uchar pTransparent[4]=NULL);

// delayTime : in 1/100 seconds
void writeGIF(const std::string &file, const std::vector<Mat> &frames, int delayTime=10, const uchar pTransparent[3]=NULL);


_CVX_END

 