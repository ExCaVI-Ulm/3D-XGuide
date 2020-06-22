#ifndef SIMPLE_BIPLANE_FILTER_H
#define SIMPLE_BIPLANE_FILTER_H

#include "BiplaneGeometry.h"
#include "CrosscorrelationFilter.h"
#include "BiplaneAlgorithm.h"
#include "PointListBasedFilter.h"
#include "MotionTrackingFilter.h"

class SimpleBiplaneFilter : public CrosscorrelationFilter, public BiplaneAlgorithm, public PointListBasedFilter, public MotionTrackingFilter
{
public:
	static SimpleBiplaneFilter* New() { return new SimpleBiplaneFilter(); }

	virtual std::vector<AbstractFilterWidget*> getPropertiesGUI();

	void overwriteMotionCorrectionMatchPosition(const int streamNumber, const double pos[2]);

protected:
	SimpleBiplaneFilter();
	virtual ~SimpleBiplaneFilter();

	virtual void ComputeOnBiplaneImages(vtkImageData* image1, vtkImageData* image2, vtkImageData* image1forDraw, vtkImageData* image2forDraw, double detectedMotion[3], std::vector<double>& detectedPositionsX, std::vector<double>& detectedPositionsY, std::vector<double>& detectedPositionsZ);
	
	virtual void pointWasAdded(unsigned int listNumber, unsigned int pointNumber);
	virtual void pointWasDeleted(unsigned int listNumber, unsigned int pointNumber);

	virtual void settingsChanged();

private:
	void computeEpilines(cv::Point2d point, int imageNumber, int pointNumber);
	cv::Vec4d checkEpipolarConstraint(unsigned int pointNumber);

	//selects the two points with min distance in 3d
	void getBestMatch(unsigned int pointNumber, cv::Point& p1, cv::Point& p2);
	void getBestMatchWithInterpolation(unsigned int imageNumber, unsigned int pointNumber, cv::Point& p1, cv::Point& p2);
	void getBestMatchWithHalfInterpolation(unsigned int imageNumber, unsigned int pointNumber, cv::Point& p1, cv::Point& p2);
	bool motionPositionSetManually;
	double manualMotionPosition[2][2];
	int motionSetManuallyStreamNumber;
	int pointUsedForManualPosition[2];

};


#endif // SIMPLE_BIPLANE_FILTER_H