#include "XRayGeometry.h"
#include "SystemGeometryDefinitions.h"
#include <qsettings.h>

using namespace std;

XRayGeometry::XRayGeometry()
: validParameters(false)
{
	focalPoint[0] = focalPoint[1] = focalPoint[2] = 0;
	isoCenter[0] = isoCenter[1] = isoCenter[2] = 0;
	positionerRotation[0] = positionerRotation[1] = 0;

	sourcePosition[0] = sourcePosition[1] = sourcePosition[2] = 0;
	detectorPosition[0] = detectorPosition[1] = detectorPosition[2] = 0;
	isoCenterPosition[0] = isoCenterPosition[1] = isoCenterPosition[2] = 0;

	patientToSourceMatrix = vtkMatrix4x4::New();
	sourceToPatientMatrix = vtkMatrix4x4::New();
	sourceToDetectorMatrix = vtkMatrix4x4::New();
	patientToDetectorMatrix = vtkMatrix4x4::New();

	//millimeterToPixel = 1.0/SystemGeometryDefinitions::MILLIMETER_PER_PIXEL;
	millimeterToPixelOld = SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[0]; // ??
	millimeterToPixel = 0.0;
	zoomFactor = 1.0;
	tablePosition[0] = tablePosition[1] = tablePosition[2] = 0.0;
	table[0] = table[1] = table[2] = 0.0;
	isFramegrabber = false;
	imageCut = 1;
}

XRayGeometry::XRayGeometry(const XRayGeometry& other)
{
	XRayGeometry(); // allocates memory etc.

	for(int i = 0; i < 3; ++i) {
		focalPoint[i] = other.focalPoint[i];
		isoCenter[i] = other.isoCenter[i];
		tablePosition[i] = other.tablePosition[i];
	}

	for (int i = 0; i < 2; ++i) {

		positionerRotation[i] = other.positionerRotation[i];
	}

	millimeterToPixel = other.millimeterToPixel;
	

	computeMatricesFromParameters();
}

XRayGeometry::~XRayGeometry() {
	if(patientToSourceMatrix) {
		patientToSourceMatrix->Delete();
		patientToSourceMatrix = 0;
	}
	if(sourceToPatientMatrix) {
		sourceToPatientMatrix->Delete();
		sourceToPatientMatrix = 0;
	}
	if(sourceToDetectorMatrix) {
		sourceToDetectorMatrix->Delete();
		sourceToDetectorMatrix = 0;
	}
	if(patientToDetectorMatrix) {
		patientToDetectorMatrix->Delete();
		patientToDetectorMatrix = 0;
	}
}

//Iso and focal are vectors pointing from the 3D image center to the
//respective point. These are not world coordinates!
void XRayGeometry::setParameters(double angles[2], double isoVector[3], double focalVector[3])
{
	for(int i = 0; i < 3; ++i) {
		isoCenter[i] = isoVector[i];
		focalPoint[i] = focalVector[i];
	}

	for (int i = 0; i < 2; ++i) {
		positionerRotation[i] = angles[i];
	}	


	computeMatricesFromParameters();
	validParameters = true;
}

void XRayGeometry::setParameters(double angles[2], double distSourceDetector, double distSourceIso)
{
	/* Assumption: vectors point directly from the image center and are
	 * exactly perpendicular to the image plane.
	 */

	double distDetectorIso = distSourceDetector - distSourceIso;

	double focalVector[3] = { 0, 0, distSourceDetector };
	double isoVector[3] = { 0, 0, distDetectorIso };
	
	setParameters(angles, isoVector, focalVector);
}


vtkMatrix4x4* XRayGeometry::generateRotationMatrix(double angles[2])
{
	const double PI = 3.141592653589793;

	/* The angles passed to this function are expected in DICOM orientation:
	LAO is (+), RAO is (-), and CRAN is (+), CAUD is (-)
	(meaning the signs of the angles). */

	// convert from degree to radiant and change sign
	const double angle1 = angles[0] * PI / 180;
	const double angle2 = -angles[1] * PI / 180;
	const double angle3 = 0.0;


	// rotate about patients y-axis ((+)LAO/(-)RAO)
	double rotY[16] = {
		cos(angle1),	0,	sin(angle1),	0,
		0,				1,	 0,				0,
		-sin(angle1),	0,	 cos(angle1),	0,
		0,				0,	 0,				1
	};

	// rotate about patients x-axis ((+)CRAN/(-)CAUD)
	double rotX[16] = {
		1,	 0,				0,				0,
		0,	 cos(angle2),	-sin(angle2),	0,
		0,	sin(angle2),	cos(angle2),	0,
		0,	0,				0,				1
	};

	// rotZ is not necessary since we does not have third angle - >anyway identity matrix
	/*double rotZ[16] = {
		cos(angle3),	 -sin(angle3),	0,	0,
		sin(angle3),	 cos(angle3),	0,	0,
		0,			0,		1,	0,
		0,			0,		0,	1
	};*/


	// Rt = (R_x*R_y)';
	double tmp1[16];
	double tmp2[16];
	double tmp3[16];
	vtkMatrix4x4::Multiply4x4(rotY, rotX, tmp1); // tmp1 := Rx * Ry
	vtkMatrix4x4::Transpose(tmp1, tmp2); // tmp2 := tmp1'
	vtkMatrix4x4* m = vtkMatrix4x4::New();
	m->DeepCopy(tmp2);

	
	return m;
}

void XRayGeometry::computeMatricesFromParameters()
{

	patientToSourceMatrix->DeepCopy(generateRotationMatrix(positionerRotation)); // set the matrix


	// translation: get isocenter position looking from focal spot
	patientToSourceMatrix->SetElement(0, 3, -isoCenter[0] + focalPoint[0]);
	patientToSourceMatrix->SetElement(1, 3, -isoCenter[1] + focalPoint[1]);
	patientToSourceMatrix->SetElement(2, 3, -isoCenter[2] + focalPoint[2]);

	// compute inverse matrix
	vtkMatrix4x4::Invert(patientToSourceMatrix, sourceToPatientMatrix);

	
	/////////////// X-ray source to detector coordinate system
	
	double cx_mm = focalPoint[0]; // image center relative to optical axis
	double cy_mm = focalPoint[1]; // image center relative to optical axis
	double sid_mm = focalPoint[2]; // source-detector distance

	//// TODO: sets the z-value... ???
	double projection[16] = {
	    sid_mm,	0,		-cx_mm,		0,
		0,		sid_mm,	-cy_mm,		0,
		0,		0,		 1,			0,
		0,		0,		 0,			1
	};


	sourceToDetectorMatrix->DeepCopy(projection);

	/////////////// patient to detector coordinate system
	
	// P = K*Rt
	vtkMatrix4x4::Multiply4x4(sourceToDetectorMatrix, patientToSourceMatrix, patientToDetectorMatrix);

	double pos[4] = {0, 0, 0, 1}; // source position in source coordinates
	sourceToPatientMatrix->MultiplyPoint(pos, pos); // transform
	// source position in the patient coordinate system	

	
	double x,y,z;
	getTablePositionInWC(x, y, z);

	sourcePosition[0] = pos[0] - x;
	sourcePosition[1] = pos[1] - y;
	sourcePosition[2] = pos[2] - z;

	// detector position in source coordinates
	pos[0] = focalPoint[0];
	pos[1] = focalPoint[1];
	pos[2] = focalPoint[2];
	pos[3] = 1;
	sourceToPatientMatrix->MultiplyPoint(pos, pos); // transform
	// detector position in the patient coordinate system
	detectorPosition[0] = pos[0] - x;
	detectorPosition[1] = pos[1] - y;
	detectorPosition[2] = pos[2] - z;

	// iso-center position in source coordinates
	pos[0] = focalPoint[0] - isoCenter[0];
	pos[1] = focalPoint[1] - isoCenter[1];
	pos[2] = focalPoint[2] - isoCenter[2];
	pos[3] = 1;
	sourceToPatientMatrix->MultiplyPoint(pos, pos); // transform
	// iso-center position in the patient coordinate system
	isoCenterPosition[0] = pos[0];
	isoCenterPosition[1] = pos[1];
	isoCenterPosition[2] = pos[2];

	
}


void XRayGeometry::setIsFramegrabber(bool isFramegrabberp) {

	isFramegrabber = isFramegrabberp;
}

void XRayGeometry::setMillimeterToPixelScaling(double mmPerPixel)
{
	millimeterToPixel = 1.0/mmPerPixel;
	millimeterToPixelOld = mmPerPixel;
}

void XRayGeometry::setZoomFactor(double factor)
{
	zoomFactor = factor;
}


void XRayGeometry::setTablePosition(double lateralPos, double longitudinalPos, double verticalPos)
{
	tablePosition[0] = lateralPos;
	tablePosition[1] = longitudinalPos;
	tablePosition[2] = verticalPos;
	
}

void XRayGeometry::getTablePosition(double& positionX, double& positionY, double& positionZ)
{
	positionX = tablePosition[0];
	positionY = tablePosition[1];
	positionZ = tablePosition[2];

}

void XRayGeometry::setImageDimension(int width, int height, bool cut)
{
	imageWidth = width;
	imageHeight = height;
	imageCut = cut;

}

void XRayGeometry::getImageDimension(int& width, int& height, bool& cut) const
{
	width = imageWidth;
	height = imageHeight;
	cut = imageCut;
}

void XRayGeometry::getPositionerAngles(double& rot, double& ang) const
{
	rot = positionerRotation[0]; ang = positionerRotation[1];
}


vtkMatrix4x4* XRayGeometry::getSourceToPatientMatrix() const
{
	vtkMatrix4x4* copy = vtkMatrix4x4::New();
	copy->DeepCopy(sourceToPatientMatrix);

	return copy;
}

vtkMatrix4x4* XRayGeometry::getPatientToSourceMatrix() const
{
	vtkMatrix4x4* copy = vtkMatrix4x4::New();
	copy->DeepCopy(patientToSourceMatrix);

	return copy;
}

void XRayGeometry::transformFromImageToDetectorCoordinates(double& x, double& y) const
{
	// the origin was the corner and is the center now
	if (!isFramegrabber) {
		x -= (imageWidth / 2);
	}
	else if (!imageCut)
	{
		x -= (imageWidth / 2 + SystemGeometryDefinitions::CLIP_X_MIN); //only for inputFromFramegrabber
	}
	else
	{
		x -= (imageWidth / 2);
		
	};
	y -= (imageHeight/2);

	// apply scaling
	x *= getPixelToMillimeterScaling();
	y *= getPixelToMillimeterScaling();
}

void XRayGeometry::transformFromDetectorToImageCoordinates(double& x, double& y) const
{
	x *= this->getMillimeterToPixelScaling();
	y *= this->getMillimeterToPixelScaling();

	if (!isFramegrabber) {
		x += imageWidth / 2;
	}
	else if (!imageCut)
	{
		x += imageWidth / 2 + SystemGeometryDefinitions::CLIP_X_MIN;	//only for inputFromFramegrabber
	}
	else
	{
		x += imageWidth / 2;
	};
	y += imageHeight / 2;
}

void XRayGeometry::transformFromImageToDetectorCoordinates(double pt[2]) const
{
	this->transformFromImageToDetectorCoordinates(pt[0],pt[1]);
}

void XRayGeometry::transformFromDetectorToImageCoordinates(double pt[2]) const
{
	this->transformFromDetectorToImageCoordinates(pt[0],pt[1]);
}

void XRayGeometry::transformFromSourceToPatientCoordinates(double pt[3]) const
{
	double p[4] = {pt[0], pt[1], pt[2], 1.0};
	sourceToPatientMatrix->MultiplyPoint(p, p);
	pt[0] = p[0]/p[3];
	pt[1] = p[1]/p[3];
	pt[1] = p[2]/p[3];
}

void XRayGeometry::transformFromImageToPatientCoordinates(double pt[3]) const
{
	//first convert to detector coordinates
	double p_det[2];
	p_det[0] = pt[0];
	p_det[1] = pt[1];
	
	this->transformFromImageToDetectorCoordinates(p_det);

	//convert 2d detector coordinates point into world coordinates
	double detectorOrigin1[4] = {
		-sourceToDetectorMatrix->GetElement(0, 2), // mm_x
		-sourceToDetectorMatrix->GetElement(1, 2), // mm_y
		sourceToDetectorMatrix->GetElement(0, 0), // SID
		1
	};

	double pos[4] = {
		detectorOrigin1[0] + p_det[0],
		detectorOrigin1[1] + p_det[1],
		detectorOrigin1[2],
		detectorOrigin1[3]
	};
	this->sourceToPatientMatrix->MultiplyPoint(pos, pos); // pos now is p in patient coordinates
	
	double x, y, z;
	getTablePositionInWC(x, y, z);
	pt[0] = pos[0] - x;
	pt[1] = pos[1] - y;
	pt[2] = pos[2] - z;	
}

void XRayGeometry::project3dPointToImageCoordinates(const double _pt3d[3], double _pt2d[2]) const
{
	this->project3dPointToDetectorCoordinates(_pt3d,_pt2d);
	this->transformFromDetectorToImageCoordinates(_pt2d[0],_pt2d[1]);
}

void XRayGeometry::project3dPointToDetectorCoordinates(const double _pt3d[3], double _pt2d[2]) const
{
	double x, y, z;
	getTablePositionInWC(x, y, z);

	double pt[4];
	pt[0] = _pt3d[0] + x;
	pt[1] = _pt3d[1] + y;
	pt[2] = _pt3d[2] + z;
	pt[3] = 1.0;
	patientToDetectorMatrix->MultiplyPoint(pt,pt);
	_pt2d[0] = pt[0] / pt[2];
	_pt2d[1] = pt[1] / pt[2];
}

void XRayGeometry::project3dVectorToImageCoordinates(const double vec[3], double res[2]) const
{
	// project origin
	double nullPos[3] = {0, 0, 0};
	double nullProjected[2];
	project3dPointToImageCoordinates(nullPos, nullProjected);

	// project the point on which the vector points
	double pointProjected[2];
	project3dPointToImageCoordinates(vec, pointProjected);

	// construct vector
	for(int i = 0; i < 2; ++i) {
		res[i] = pointProjected[i] - nullProjected[i];
	}
}

void XRayGeometry::getSourcePosition(double& x, double& y, double& z) const
{
	x = sourcePosition[0]; y = sourcePosition[1]; z = sourcePosition[2];
}

void XRayGeometry::getDetectorPosition(double& x, double& y, double& z) const
{
	x = detectorPosition[0]; y = detectorPosition[1]; z = detectorPosition[2];
}

void XRayGeometry::getTablePositionInWC(double& x, double& y, double& z) const
{
	
	x = tablePosition[0] - SystemGeometryDefinitions::TABLE_POSITION_X;
	y = tablePosition[1] - SystemGeometryDefinitions::TABLE_POSITION_Y;
	z = tablePosition[2] - SystemGeometryDefinitions::TABLE_POSITION_Z;
	if (isFramegrabber)
	{
		// shift = sys-osd -> sys = osd+shift
		x += SystemGeometryDefinitions::TABLE_SHIFT_CS_POSITION_X; // lat
		y += SystemGeometryDefinitions::TABLE_SHIFT_CS_POSITION_Y; // long
		z += SystemGeometryDefinitions::TABLE_SHIFT_CS_POSITION_Z; // vert		
	}
	
}

void XRayGeometry::getIsoCenterPosition(double& x, double& y, double& z) const
{
	x = isoCenterPosition[0]; y = isoCenterPosition[1]; z = isoCenterPosition[2];
}

void XRayGeometry::getIsoCenterVector(double& x, double& y, double& z) const
{
	x = isoCenter[0]; y = isoCenter[1]; z = isoCenter[2];
}