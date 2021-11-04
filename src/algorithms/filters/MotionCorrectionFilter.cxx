#include "MotionCorrectionFilter.h"

#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkDemandDrivenPipeline.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkInformationVector.h>
#include <vtkAssembly.h>
#include <vtkMatrix4x4.h>

#include "MotionCorrectionFilterWidget.h"
#include "XRayGeometry.h"

using namespace std;

MotionCorrectionFilter::MotionCorrectionFilter()
: GenericMotionCorrectionFilter()
{
	SetNumberOfInputPorts(1);
	SetNumberOfOutputPorts(1);

	setReferenceFrame = false;
	resetReferenceFrame = false;


	geometry = 0;
}

MotionCorrectionFilter::~MotionCorrectionFilter()
{
}

void MotionCorrectionFilter::setGeometry(XRayGeometry* g)
{
	if(g != geometry) {
		geometry = g;
		Modified();
	}
}

int MotionCorrectionFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
	info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
	return 1;
}

int MotionCorrectionFilter::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
	// image output
	info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
	return 1;	
}

void MotionCorrectionFilter::SetInput(int index, vtkImageData* input)
{

	if(input != 0) {
		this->SetInputDataObject(index, input);
		//this->SetInputConnection(index, input->GetProducerPort());


	}
	else {
		//this->SetInputConnection(index, 0); // Setting a NULL input removes the connection.
		this->SetInputDataObject(index, 0);
	}
}

vtkImageData* MotionCorrectionFilter::GetImageOutput()
{
	return vtkImageData::SafeDownCast(this->GetOutputDataObject(0));
}


int MotionCorrectionFilter::RequestDataObject(
	vtkInformation*, 
	vtkInformationVector** inputVector , 
	vtkInformationVector* outputVector)
{
	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	vtkImageData* outputImage = vtkImageData::SafeDownCast( outInfo->Get(vtkDataObject::DATA_OBJECT()) );

	// generate default output objects, if not already existing
	if(!outputImage) {
		outputImage = vtkImageData::New();	
		vtkInformation* outInfo = outputVector->GetInformationObject(0);
		vtkDataObject::SetPointDataActiveScalarInfo(
			outInfo, VTK_UNSIGNED_CHAR, 1);

		outputImage->SetDimensions(0, 0, 0);		
		////outputImage->SetPipelineInformation(outInfo1);
		//outputImage->Delete();
		outputImage->Delete();
		}			

	return 1;
}

int MotionCorrectionFilter::RequestInformation(
	vtkInformation* request,
	vtkInformationVector** inputVector,
	vtkInformationVector* outputVector)
{
	vtkInformation* outInfoImage = outputVector->GetInformationObject(0);
	outInfoImage->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
	
	return 1;
}

int MotionCorrectionFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),	vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
	// request whole image
	vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
	inputInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

	return 1;
}

int MotionCorrectionFilter::RequestData(
	vtkInformation* request,
	vtkInformationVector** inputVector,
	vtkInformationVector* outputVector)
{
	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	vtkImageData* outputImage = vtkImageData::SafeDownCast( outInfo->Get(vtkDataObject::DATA_OBJECT()) );	

	vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
	vtkImageData* inputImage = vtkImageData::SafeDownCast( inInfo->Get(vtkDataObject::DATA_OBJECT()) );
	
	outputImage->ShallowCopy(inputImage);// just pass the image
		
	vtkImageData* tmpImage = vtkImageData::New();
	tmpImage->ShallowCopy(inputImage);


	if(ProcessImages) {

		clock_t time_start, time_stop;
		double duration;
		f.open("ComputeMotionTime.txt", ios::out | ios::app);
		time_start = clock();	

		ComputeMotion(inputImage, MotionX, MotionY); // sets the last parameters
		// projects motion to 2D and apply it
		time_stop = clock();
		duration = (double)(time_stop - time_start);
		f << duration << endl;
		f.close();
		Apply2dMotion();

	}

	if (!setReferenceFrame)
	{
		referenceImage = tmpImage;
	}

	if (!resetReferenceFrame)
	{
		referenceImageNew = tmpImage;
	}		
	else
	{
		referenceImage = referenceImageNew;
		resetReferenceFrame = false;
	}
		


	return 1;
}

void MotionCorrectionFilter::Apply2dMotion()
{
	const double distSourceDetector = geometry->getSourceDetectorDistance();

	double motionX = MotionX;
	double motionY = MotionY;

	// apply shift and scale
	motionX += ShiftX;
	motionY += ShiftY;

	if(UseManualScaling) {
		motionX *= ScaleX;
		motionY *= ScaleY;
	}

	// transform from image to world coordinates
	motionX *= ImageToWorldCoordinatesScaling;
	motionY *= ImageToWorldCoordinatesScaling;

	double distSourceMesh;
	double scale, scaleMesh;

	// apply motion vectors
	for(unsigned int i = 0; i < AssembliesToMove.size(); ++i) {
		// inverse movement only in 3D motion correction
		if(InvertAssemblyMovement[i]) continue;

		vtkMatrix4x4* matrix = AssembliesToMove[i]->GetUserMatrix();
		if(!matrix) {
			cout << __FILE__ << " " << __LINE__ << ": no user matrix!" << endl;
			continue;
		}

		double pos[3] = {
			matrix->Element[0][3],
			matrix->Element[1][3],
			matrix->Element[2][3]
		};

		// apply "Strahlensatz" (intercept theorem) on the motion correction
		// for the current assembly
		//if(!UseManualScaling) {
		//	// In 2D view, the z-axis points from the iso-center to the source.
		//	// In the source coordinate system of geometry,
		//	// the z-axis points from the source to the detector.
		//	//pos[2] = -pos[2] + geometry->getIsoCenterToSourceDistance();

		//	distSourceMesh = vtkMath::Norm(pos);
		//	scale = distSourceMesh/distSourceDetector;

		//	if(i ==0) { scaleMesh = scale; }
		//	if(i == 1) { scale = 1/scaleMesh; }

		//	motionX *= scale;
		//	motionY *= scale;
		//}

		// set translation components in user matrix
		// to the null position + the measured movement
		// 2D motion is only applied to not inverted motion correction
		if(MoveHorizontally) matrix->SetElement(0, 3, AssemblyNullPositionsX[i] + motionX);
		if(MoveVertically) matrix->SetElement(1, 3, AssemblyNullPositionsY[i] + motionY);

		AssembliesToMove[i]->Modified();
	}//for i
}


void MotionCorrectionFilter::ComputeMotion(vtkImageData* image, double& resultingMotionX, double& resultingMotionY/*, vector<double>& resultingPositionsX, vector<double>& resultingPositionsY*/)
{
	// No motion in this default implementation.
	resultingMotionX = 0.0;
	resultingMotionY = 0.0;

	//// No position (i.e. 0.0, 0.0), either.
	//resultingPositionsX.clear();
	//resultingPositionsX.push_back(0.0);
	//resultingPositionsY.clear();
	//resultingPositionsY.push_back(0.0);
}


vector<AbstractFilterWidget*> MotionCorrectionFilter::getPropertiesGUI()
{
	return this->GenericMotionCorrectionFilter::getPropertiesGUI();
}