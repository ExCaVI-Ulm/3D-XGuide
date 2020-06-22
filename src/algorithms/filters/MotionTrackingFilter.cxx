#include "MotionTrackingFilter.h"
#include <opencv2/imgproc.hpp>


MotionTrackingFilter::MotionTrackingFilter()
{
}

MotionTrackingFilter::~MotionTrackingFilter()
{
}

void MotionTrackingFilter::trackMotionCorrectionPoint(cv::Mat &matchImg, cv::Mat &tmpl, cv::Point &matchPosition, int width, int height)
{
	cv::Mat correlation(height, width, CV_8U);	
	matchTemplate(matchImg, tmpl, correlation, CV_TM_CCOEFF_NORMED);
			// find the maximum
	minMaxLoc(correlation, NULL, NULL, NULL, &matchPosition);

}