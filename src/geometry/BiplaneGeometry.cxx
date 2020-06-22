#include "BiplaneGeometry.h"
#include <vtkMath.h> 

BiplaneGeometry::BiplaneGeometry()
: validParameters(false)
{

}

BiplaneGeometry::BiplaneGeometry(const BiplaneGeometry& other)
{
	BiplaneGeometry();

	firstSystem = other.firstSystem;
	secondSystem = other.secondSystem;
	mainSystem = other.mainSystem;

	computeParameters();
}

void BiplaneGeometry::computeParameters()
{
	validParameters = false;

	firstSystem.computeMatricesFromParameters();
	secondSystem.computeMatricesFromParameters();

	// THE FOLLOWING CALL HAMPERS RECOMPUTATION OF THE GEOMETRY!!!!?
	//if(!firstSystem.isValid() || !secondSystem.isValid()) return;

	////////////// compute fundamental matrices //////////////
	double epipole1[4], epipole2[4];

	double firstSource[4] = {firstSystem.sourcePosition[0], firstSystem.sourcePosition[1], firstSystem.sourcePosition[2], 1};
	double secondSource[4] = {secondSystem.sourcePosition[0], secondSystem.sourcePosition[1], secondSystem.sourcePosition[2], 1};
	firstSystem.patientToDetectorMatrix->MultiplyPoint(secondSource, epipole1);
	secondSystem.patientToDetectorMatrix->MultiplyPoint(firstSource, epipole2);

	cv::Mat epipoleMat1 = (cv::Mat_<float>(3, 3)<<
		0, -epipole1[2], epipole1[1],
		epipole1[2], 0, -epipole1[0],
		-epipole1[1], epipole1[0], 0);
	
	cv::Mat epipoleMat2 = (cv::Mat_<float>(3, 3)<<
		0, -epipole2[2], epipole2[1],
		epipole2[2], 0, -epipole2[0],
		-epipole2[1], epipole2[0], 0);

	// get the projection matrices (P = K*Rt) for both systems
	cv::Mat patientToDetectorMatrixMat1(3, 4, CV_32F);
	cv::Mat patientToDetectorMatrixMat2(3, 4, CV_32F);

	for (unsigned int i = 0; i < 3; i++){
		for (unsigned int j = 0; j < 4; j++){
			patientToDetectorMatrixMat1.at<float>(i, j) =
			firstSystem.patientToDetectorMatrix->Element[i][j];
			patientToDetectorMatrixMat2.at<float>(i, j) =
			secondSystem.patientToDetectorMatrix->Element[i][j];
		}
	}
	
	// compute fundamental matrix: e2*P2*P1+
	firstFundamentalMatrix = epipoleMat2 * patientToDetectorMatrixMat2 * patientToDetectorMatrixMat1.inv(cv::DECOMP_SVD);
	// compute fundamental matrix: e1*P1*P2+
	secondFundamentalMatrix = epipoleMat1 * patientToDetectorMatrixMat1 * patientToDetectorMatrixMat2.inv(cv::DECOMP_SVD);

	//////////////////////// cleanup ////////////////////////

	validParameters = true;
}

void BiplaneGeometry::getFundamentalMatrix(cv::Mat& fundamentalMatrix, const int ndx) const
{
	switch(ndx)
	{
	case 1:
		fundamentalMatrix = firstFundamentalMatrix;
		break;
	case 2:
		fundamentalMatrix = secondFundamentalMatrix;
		break;
	default:
		std::cout << "ERROR: Parameter error! Only two fundamental matrices!" << std::endl;
		break;
	}
}

double BiplaneGeometry::reconstruct3dPointFromImageCoordinates(
		const double p1[2], const double p2[2],
		double res[3], ReconstructionMethod method ) const
{
	// -- convert to detector coordinates first
	double p1_det[2], p2_det[2];
	p1_det[0] = p1[0];
	p1_det[1] = p1[1];
	p2_det[0] = p2[0];
	p2_det[1] = p2[1];
	firstSystem.transformFromImageToDetectorCoordinates(p1_det);
	secondSystem.transformFromImageToDetectorCoordinates(p2_det);

	return this->reconstruct3dPointFromDetectorCoordinates(p1_det,p2_det,res,method);
}

double BiplaneGeometry::reconstruct3dPointFromDetectorCoordinates(
		const double p1[2], const double p2[2],
		double res[3], ReconstructionMethod method ) const
{
	switch(method)
	{
	case REC_SKEWLINESINTERSECTION:
		return this->reconstruct3dPointWithSkewLinesIntersection(p1,p2,res);
		break;
	case REC_LINEARTRIANGSVD:
		std::cout << "ERROR: reconstruction mode REC_LINEARTRIANGSVD not implemented yet!" << std::endl;
		return -1;
		break;
	default:
		std::cout << "ERROR: reconstruction mode not supported!" << std::endl;
		return -1;
	}
}

double BiplaneGeometry::reconstruct3dPointWithSkewLinesIntersection(const double p1[2], const double p2[2], double res[3]) const
{
	double x, y, z;
	///////////////// convert 2d detector coordinates point into world coordinates
	double detectorOrigin1[4] = {
		-firstSystem.sourceToDetectorMatrix->GetElement(0, 2), // mm_x
		-firstSystem.sourceToDetectorMatrix->GetElement(1, 2), // mm_y
		firstSystem.sourceToDetectorMatrix->GetElement(0, 0), // SID
		1
	};

	double pos1[4] = {
		detectorOrigin1[0] + p1[0],
		detectorOrigin1[1] + p1[1],
		detectorOrigin1[2],
		detectorOrigin1[3]
	};
	firstSystem.sourceToPatientMatrix->MultiplyPoint(pos1, pos1); // pos1 now is p1 in patient coordinates
	
		firstSystem.getTablePositionInWC(x, y, z);

		pos1[0] = pos1[0] - x;
		pos1[1] = pos1[1] - y;
		pos1[2] = pos1[2] - z;


	// projection from source to the point
	double projection1[3] = {
		pos1[0] - firstSystem.sourcePosition[0],
		pos1[1] - firstSystem.sourcePosition[1],
		pos1[2] - firstSystem.sourcePosition[2]
	};

	double detectorOrigin2[4] = {
		-secondSystem.sourceToDetectorMatrix->GetElement(0, 2), // mm_x
		-secondSystem.sourceToDetectorMatrix->GetElement(1, 2), // mm_y
		secondSystem.sourceToDetectorMatrix->GetElement(0, 0), // SID
		1
	};

	double pos2[4] = {
		detectorOrigin2[0] + p2[0],
		detectorOrigin2[1] + p2[1],
		detectorOrigin2[2],
		detectorOrigin2[3]
	};
	secondSystem.sourceToPatientMatrix->MultiplyPoint(pos2, pos2); // pos2 now is p2 in patient coordinates

		secondSystem.getTablePositionInWC(x, y, z);

		pos2[0] = pos2[0] - x;
		pos2[1] = pos2[1] - y;
		pos2[2] = pos2[2] - z;

	// projection from source to the point
	double projection2[3] = {
		pos2[0] - secondSystem.sourcePosition[0],
		pos2[1] - secondSystem.sourcePosition[1],
		pos2[2] - secondSystem.sourcePosition[2]
	};


	// cross product directs to shortest link between projection lines
	double crossProduct[3];
	vtkMath::Cross(projection1, projection2, crossProduct);

	// The distance vector between the two sources may also
	// be written as the linear combination of the projections and their cross product.
	// Thus, solve the equation system:
	double eqCoefficients[3][3] = {
		{ -projection1[0],	projection2[0],	crossProduct[0] },
		{ -projection1[1],	projection2[1],	crossProduct[1] },
		{ -projection1[2],	projection2[2],	crossProduct[2] }
	};
	double eqInverted[3][3];
	vtkMath::Invert3x3(eqCoefficients, eqInverted);

	double source2tosource1[3] = {
		firstSystem.sourcePosition[0] - secondSystem.sourcePosition[0],
		firstSystem.sourcePosition[1] - secondSystem.sourcePosition[1],
		firstSystem.sourcePosition[2] - secondSystem.sourcePosition[2]
	};

	// parameters holds the solution of the equation system
	double parameters[3];
	vtkMath::Multiply3x3(eqInverted, source2tosource1, parameters);

	// nearest point on first projection
	double point1[3] = {
		firstSystem.sourcePosition[0] +  parameters[0]*projection1[0],
		firstSystem.sourcePosition[1] +  parameters[0]*projection1[1],
		firstSystem.sourcePosition[2] +  parameters[0]*projection1[2],
	};

	// nearest point on second projection
	double point2[3] = {
		secondSystem.sourcePosition[0] +  parameters[1]*projection2[0],
		secondSystem.sourcePosition[1] +  parameters[1]*projection2[1],
		secondSystem.sourcePosition[2] +  parameters[1]*projection2[2],
	};

	// take the middle point
	res[0] = (point1[0] + point2[0]) / 2;
	res[1] = (point1[1] + point2[1]) / 2;
	res[2] = (point1[2] + point2[2]) / 2;

	double vec[3] = {
		point2[0] - point1[0],
		point2[1] - point1[1],
		point2[2] - point1[2]
	};

	// The vtkMath function returns the squared distance; thus sqrt is needed.
	double distance = sqrt(vtkMath::Distance2BetweenPoints(point1, point2));
	return distance;
}

void BiplaneGeometry::reconstruct3dVectorFromImageCoordinatesVector(double v1[2], double v2[2], double res[3], ReconstructionMethod method) const
{
	double nullPoint[2] = {0, 0};
	double null3d[3];
	double point3d[3];

	switch(method) {
		case REC_SKEWLINESINTERSECTION:
			// reconstruct origin
			reconstruct3dPointFromImageCoordinates(nullPoint, nullPoint, null3d, REC_SKEWLINESINTERSECTION);

			// reconstruct the point on which the vector is pointing
			reconstruct3dPointFromImageCoordinates(v1, v2, point3d, REC_SKEWLINESINTERSECTION);

			// build 3D vector as result
			for(int i = 0; i < 3; ++i) {
				res[i] = point3d[i] - null3d[i];
			}

			break;
		default:
			cout << __FILE__ << " " << __LINE__ << ": Requested vector reconstruction method not implemented" << endl;
			return;
	}
}