#pragma once

#include"re3d/base/base.h"

namespace redist
{
	using namespace re3d;

	double elapsed();

	cv::Mat renderResults(const cv::Mat &img, cv::Mat1f& returnDepth, FrameData &fd, ModelSet &ms, bool _drawContour = true, bool _drawBlend = true, bool _drawFrame = true, bool _drawScore=true);

    cv::Mat renderResults(const cv::Mat &img, FrameData &fd, ModelSet &ms, bool _drawContour = true, bool _drawBlend = true, bool _drawFrame = true, bool _drawScore=true);

}


