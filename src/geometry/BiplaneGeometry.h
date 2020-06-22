#ifndef BIPLANE_GEOMETRY_H
#define BIPLANE_GEOMETRY_H

#include "XRayGeometry.h"
#include <cv.h>

//using namespace cv;

/** Class for 3D geometry information.
    Holds three XRayGeometry instances, one for each X-Ray tube, plus one for the actual run.
  */
class BiplaneGeometry {
public:
	enum ReconstructionMethod
	  {REC_SKEWLINESINTERSECTION,REC_LINEARTRIANGSVD};

	BiplaneGeometry();
	BiplaneGeometry(const BiplaneGeometry& other);

	void computeParameters(); // computes the values for each system and the biplane parameters

	bool isValid() const { return validParameters; }

	/// Reconstruct a 3D point given two points in detector coordinates.
	/// 3D point in patient coordinate system is written in res.
	/// Distance between the two nearest points is returned.
	double reconstruct3dPointWithSkewLinesIntersection(
			const double p1[2], const double p2[2], double res[3]) const;
	double reconstruct3dPointFromImageCoordinates(const double p1[2], const double p2[2],
			double res[3], ReconstructionMethod method=REC_SKEWLINESINTERSECTION ) const;
	double reconstruct3dPointFromDetectorCoordinates(const double p1[2], const double p2[2],
			double res[3], ReconstructionMethod method=REC_SKEWLINESINTERSECTION ) const;

	void reconstruct3dVectorFromImageCoordinatesVector(double v1[2], double v2[2], double res[3], ReconstructionMethod method=REC_SKEWLINESINTERSECTION) const;

	/// the user may access the geometry for each system
	/// but is responsible for a biplane update after changing something.
	XRayGeometry firstSystem;
	XRayGeometry secondSystem;
	XRayGeometry mainSystem;

	// getter for the fundamental matrix
	void getFundamentalMatrix(cv::Mat& fundamentalMatrix, const int ndx=1) const;

private:
	bool validParameters;	

	// the fundamental matrices
	cv::Mat firstFundamentalMatrix;
	cv::Mat secondFundamentalMatrix;

};


#endif
