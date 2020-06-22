#ifndef CROSSCORRELATION_FILTER_H
#define CROSSCORRELATION_FILTER_H

#include <vector>
//using namespace std;
#include <cv.h>
//using namespace cv;
#include <list>

class AbstractFilterWidget;

/** For filters using template matching with crosscorrelation.
    This class holds all parts for crosscorrelation which are independent of
	the number of images they operate on.
 */

class CrosscorrelationFilter;

class CrosscorrelationFilter
{
public:
	static CrosscorrelationFilter* New() { return new CrosscorrelationFilter(); }

	typedef std::vector<cv::Mat> myMatVector;
	typedef std::vector<myMatVector> myMatVectorVector;

	typedef std::vector<cv::Point> myPointVector;
	typedef std::vector<myPointVector> myPointVectorVector;

	typedef std::vector<cv::Point2d> myPoint2dVector;
	typedef std::vector<myPoint2dVector> myPoint2dVectorVector;
	typedef std::vector<myPoint2dVectorVector> myPoint2dVectorVectorVector;

	typedef std::vector<cv::Point2f> myPoint2fVector;
	typedef std::vector<myPoint2fVector> myPoint2fVectorVector;
	typedef std::vector<myPoint2fVectorVector> myPoint2fVectorVectorVector;

	// container for local correlation maxima
	struct PointCorrelation {
		cv::Point2d point; // point coordinates
		double corr; // correlation value
		double dist; // distance to corresponding epipolar line
	};

	/// sort the correlation list by correlation values (highest -> lowest)
	struct comparePointCorrelationByCorr {
	  bool operator()(const PointCorrelation& first, const PointCorrelation& second) const {
		  return first.corr > second.corr;
	  }
	};
	/// sort the correlation list by distance values (shortest -> longest)
	struct comparePointCorrelationByDist {
	  bool operator()(const PointCorrelation& first, const PointCorrelation& second) const {
		  return first.dist < second.dist;
	  }
	};

	// getter and setter for filter settings
	void setnumberOfRotations(int number);
	int getnumberOfRotations() const;
	// use region of interest
	void setUseROI(bool isChecked);
	bool getUseROI() const;
	// use restriction in y-direction
	void setRestrictY(bool isChecked);
	bool getRestrictY() const;
	// the template size
	void setTplSize(int x, int y);
	int getTplSize(int index) const;
	// ROI = tplSize * factorROISize
	void setFactorROISize(double factor);
	double getFactorROISize() const;
	// basic template = template cutted out from image
	void setUseBasicTpl(bool isChecked);
	bool getUseBasicTpl() const;
	// use as template a given model
	void setUseModelBasedTpl(bool isChecked);
	bool getUseModelBasedTpl() const;
	// markers (roi, tpl, tip) visible
	void setMarkersVisible(bool isChecked);
	bool getMarkersVisible() const;
	// use usharp masking
	/*void setUseUnsharpMasking(bool isChecked);
	bool getUseUnsharpMasking() const;

	void setUnsharpMaskingSigma(double factor);
	double getUnsharpMaskingSigma() const;
	void setUnsharpMaskingWeight(double factor);
	double getUnsharpMaskingWeight() const;*/

protected:
	CrosscorrelationFilter(int numberOfImages = 1);
	virtual ~CrosscorrelationFilter();
	virtual void settingsChanged();
	void setROI(cv::Point position, cv::Mat img,  unsigned int imageNumber, unsigned int pointNumber);
	void setTemplates(cv::Mat img, cv::Point point, int pointType, unsigned int imageNumber, unsigned int pointNumber);
	void rotateTemplate(const CvMat* src, float angle, int pointNumber, CvMat* res);
	cv::Point getBestMaxLoc(cv::Mat correlation, unsigned int imageNumber, unsigned int pointNumber);
	void drawMarkers(cv::Mat img1, cv::Mat img2, int pointType, cv::Point2d matchPosition, unsigned int imageNumber, unsigned int pointNumber);
	std::vector<cv::Vec3f> getCorrespondingEpiline(cv::Mat fundamentalMatrix, cv::Point point, unsigned int imageNumber);
	void setEpilinePoints(std::vector<cv::Point2d> epiLinePoints, unsigned int imageNumber, unsigned int pointNumber);
	void setGoodFeaturesToTrack(cv::Mat img, unsigned int imageNumber, unsigned int pointNumber);
	void cleanUp(unsigned int listNumber, unsigned int pointNumber);
	void checkAndAdjustRange(cv::Mat img, int& yStart, int& yEnd, int& xStart, int& xEnd);
	void findLocalMaxima(cv::Mat correlation, int imageNumber, int pointNumber, bool addOffset, int pointType);

	int numberOfRotations, templateSizeX, templateSizeY;
	double factorROISize, /*maskingSigma, maskingAmount,*/ threshold;
	bool useROI, useCensus, restrictY, useBasicTpl, useModelBasedTpl, markersVisible, tplSettingsModified/*, useUnsharpMasking*/;
	myMatVectorVector correlationTemplates;
	myMatVectorVector templatesRoi;
	myPointVectorVector theOffset;
	myPoint2dVectorVector lastPosition;
	myPoint2dVectorVector measurementPosition;
	myPoint2dVectorVectorVector corrLines;
	myPoint2fVectorVectorVector theCorners;
	myMatVectorVector rotationTemplates;

	// stores the correlation maxima
	std::vector< std::vector < std::list<PointCorrelation> > > correlationList;
	//std::vector< vector < std::vector<Point2d> > > correlationList;
	//myPoint2dVectorVectorVector correlationList;
	bool isInitialization;
private:
	
};

#endif // CROSSCORRELATION_FILTER_H
