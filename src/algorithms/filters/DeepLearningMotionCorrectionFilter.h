#ifndef DEEPLEARNING_MOTION_CORRECTION_FILTER_H
#define	DEEPLEARNING_MOTION_CORRECTION_FILTER_H

#include "CrosscorrelationFilter.h"
#include "MotionCorrectionFilter.h"
#include "PointListBasedFilter.h"
#include "MotionTrackingFilter.h"

//#include "DebugTools.h"

class vtkImageData;

#include <vector>
//using namespace std;

#include <cv.h>
//using namespace cv;


class DeepLearningMotionCorrectionFilter : public CrosscorrelationFilter, public MotionCorrectionFilter, public PointListBasedFilter, public MotionTrackingFilter
{
public:
	static DeepLearningMotionCorrectionFilter* New() { return new DeepLearningMotionCorrectionFilter(); }

protected:
	DeepLearningMotionCorrectionFilter();
	virtual ~DeepLearningMotionCorrectionFilter();

	virtual void ComputeMotion(vtkImageData* image, double& resultingMotionX, double& resultingMotionY);
	virtual void pointWasAdded(unsigned int listNumber, unsigned int pointNumber);
	virtual void pointWasDeleted(unsigned int listNumber, unsigned int pointNumber);

	virtual void settingsChanged();

	virtual std::vector<AbstractFilterWidget*> getPropertiesGUI();

private:
	//size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s);
	

};

#endif //DEEPLEARNING_MOTION_CORRECTION_FILTER_H