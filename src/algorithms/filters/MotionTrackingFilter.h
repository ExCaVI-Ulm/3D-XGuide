#ifndef MOTION_TRACKING_FILTER_H
#define MOTION_TRACKING_FILTER_H

#include <cv.h>
//using namespace cv;

class MotionTrackingFilter
{
public:
	static MotionTrackingFilter* New() { return new MotionTrackingFilter(); }

protected:
	MotionTrackingFilter();
	virtual ~MotionTrackingFilter();

	void trackMotionCorrectionPoint(cv::Mat& matchImg, cv::Mat &tmpl, cv::Point& matchPosition, int width, int height);

private:

};

#endif // MOTION_TRACKING_FILTER_H