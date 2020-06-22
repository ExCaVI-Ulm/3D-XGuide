#ifndef CROSSCORRELATION_MOTION_CORRECTION_FILTER_H
#define	CROSSCORRELATION_MOTION_CORRECTION_FILTER_H

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

class CrosscorrelationMotionCorrectionFilter : public CrosscorrelationFilter, public MotionCorrectionFilter, public PointListBasedFilter, public MotionTrackingFilter
{
public:
	static CrosscorrelationMotionCorrectionFilter* New() { return new CrosscorrelationMotionCorrectionFilter(); }

	void overwriteMotionCorrectionMatchPosition(const double pos[2]);

protected:
	CrosscorrelationMotionCorrectionFilter();
	virtual ~CrosscorrelationMotionCorrectionFilter();

	virtual void ComputeMotion(vtkImageData* image, double& resultingMotionX, double& resultingMotionY);

	virtual void pointWasAdded(unsigned int listNumber, unsigned int pointNumber);
	virtual void pointWasDeleted(unsigned int listNumber, unsigned int pointNumber);

	virtual void settingsChanged();

	virtual std::vector<AbstractFilterWidget*> getPropertiesGUI();

private:
	bool motionPositionSetManually;
	double manualMotionPosition[2];
	int pointUsedForManualPosition;

};

#endif //CROSSCORRELATION_MOTION_CORRECTION_FILTER_H