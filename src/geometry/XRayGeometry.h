#ifndef X_RAY_GEOMETRY_H
#define X_RAY_GEOMETRY_H

#include <vtkMatrix4x4.h>
#include <vtkMath.h>

#include <string>

class BiplaneGeometry;

/** Class for the transformation between the different coordinate systems.
  * patient: x = right-left, y = inferior-superior, z = posterior-anterior, [mm]
  * (X-ray) source: x = left-right, y = top-down, z = source-detector, [mm]
  * detector: 2D, origin = image center, x = left-right, y = bottom-top, (z = source-detector) [mm]
  * image: 2D, origin = upper left corner, x = left-right, y = bottom-up, (z = source-detector) [pixel]
  * 
  */
class XRayGeometry {
public:
	XRayGeometry();
	XRayGeometry(const XRayGeometry& other);
	~XRayGeometry();
	
	// Iso and focal are vectors pointing from the 3D image center to the
	// respective point. These are not world coordinates!
	void setParameters(double angles[2], double isoVector[3], double focalVector[3]);
	void setParameters(double angles[2], double distSourceDetector, double distSourceIso);

	void computeMatricesFromParameters();

	/// Get the respective positions in the patient coordinate system
	void getSourcePosition(double& x, double& y, double& z) const;
	void getDetectorPosition(double& x, double& y, double& z) const;
	void getIsoCenterPosition(double& x, double& y, double& z) const;
	void getTablePositionInWC(double& x, double& y, double& z) const;

	/// Get the respective vectors in the camera coordinate system
	void getIsoCenterVector(double& x, double& y, double& z) const;

	/// transformation between different coordinate systems
	//void transformDetectorToPatientCoordinates(double position[3]);

	bool isValid() const { return validParameters; }


	void setZoomFactor(double factor);
	void setImageDimension(int width, int height); /// in [pix]
	void getImageDimension(int& width, int& height) const; /// in [pix]
	void setMillimeterToPixelScaling(double mmPerPixel); // mmPerPixel = SystemGeometryDefinitions::MILLIMETER_PER_PIXEL ( = Imager Pixel Spacing in DICOM header)
	void setTablePosition(double lateralPos, double longitudinalPos, double verticalPos);
	void getTablePosition(double& positionX, double& positionY, double& positionZ);
	double getSourceDetectorDistance() const { return vtkMath::Norm(focalPoint); }
	double getIsoCenterToDetectorDistance() const { return vtkMath::Norm(isoCenter); }
	double getIsoCenterToSourceDistance() const { 
		double tmp[3] = {
			focalPoint[0]-isoCenter[0],
			focalPoint[1]-isoCenter[1],
			focalPoint[2]-isoCenter[2]
		};
		return vtkMath::Norm(tmp);
	}

	double getMillimeterToPixelScalingOld() const { return millimeterToPixelOld; }
	double getZoomFactor() const { return zoomFactor; }
	double getMillimeterToPixelScaling() const { return millimeterToPixel; }
	double getPixelToMillimeterScaling() const { return 1.0 / millimeterToPixel; }

	void getPositionerAngles(double& rot, double& ang) const; /// in [deg]

	void transformFromImageToDetectorCoordinates(double& x, double& y) const;
	void transformFromDetectorToImageCoordinates(double& x, double& y) const;
	void transformFromImageToDetectorCoordinates(double pt[2]) const;
	void transformFromDetectorToImageCoordinates(double pt[2]) const;
	void transformFromSourceToPatientCoordinates(double pt[3]) const;
	void transformFromImageToPatientCoordinates(double pt[3]) const;

	void project3dPointToImageCoordinates(const double _pt3d[3], double _pt2d[2] ) const;
	void project3dPointToDetectorCoordinates(const double _pt3d[3], double _pt2d[2] ) const;
	void project3dVectorToImageCoordinates(const double vec[3], double res[2]) const;
	
	void setIsFramegrabber(bool isFramegrabber);

	/// @return a copy of SourceToPatientMatrix. This copy will not be updated when the original changes!
	vtkMatrix4x4* getSourceToPatientMatrix() const;

	/// @return a copy of PatientToSourceMatrix. This copy will not be updated when the original changes!
	vtkMatrix4x4* getPatientToSourceMatrix() const;

private:
	// angles given in degrees (not radians)
	vtkMatrix4x4* generateRotationMatrix(double angles[2]);

	bool validParameters; // true <=> parameters could be loaded and matrices are set
	double tablePosition[3]; // position of the XRAY TableTop
	double table[3]; // vector from table CS center to the ISO-center of the positioner
	double focalPoint[3]; // vector from image center to the focalpoint (i.e. X-Ray source)
	double isoCenter[3]; // vector from image center to the ISO-center of the positioner
	double positionerRotation[2]; // rotation angles: LAO(-)/RAO(+), CRAN(-)/CAUD(+)[deg]

	// the source position in the patient coordinate system
	double sourcePosition[3];
	// the detector position in the patient coordinate system
	double detectorPosition[3];
	// the iso-center position in the patient coordinate system
	double isoCenterPosition[3];

	double millimeterToPixel; // [pixel/mm]; mutliplied with a [mm], the result is in [pixel]
	double millimeterToPixelOld;
	double zoomFactor;

	int imageHeight, imageWidth; // height and width of the image (as it is shown) in [pix]	

	bool isFramegrabber;

	vtkMatrix4x4* patientToSourceMatrix; // Rt matrix
	vtkMatrix4x4* sourceToPatientMatrix; // Rt inverse matrix
	vtkMatrix4x4* sourceToDetectorMatrix; // K matrix
	vtkMatrix4x4* patientToDetectorMatrix; // P = K*Rt matrix

	friend class BiplaneGeometry; // may access my private stuff
};

#endif
