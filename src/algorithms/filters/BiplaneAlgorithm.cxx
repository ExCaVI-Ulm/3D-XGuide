#include "BiplaneAlgorithm.h"

#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkDemandDrivenPipeline.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkInformationVector.h>
#include <vtkTransform.h>
#include <vtkAssembly.h>

#include "BiplaneGeometry.h"
using namespace std;

BiplaneAlgorithm::BiplaneAlgorithm()
: GenericMotionCorrectionFilter()
{
	SetNumberOfInputPorts(2);
	SetNumberOfOutputPorts(2);

	UseManualScaling = false;

	biplaneGeometry = NULL;
}

BiplaneAlgorithm::~BiplaneAlgorithm()
{
}

void BiplaneAlgorithm::setBiplaneGeometry(const BiplaneGeometry* geom)
{
	if(biplaneGeometry != geom) {
		biplaneGeometry = geom;
		Modified();
	}
}

const BiplaneGeometry* BiplaneAlgorithm::getBiplaneGeometry() const
{
	return biplaneGeometry;
}

void BiplaneAlgorithm::addAssemblyToMove(vtkAssembly* assembly, bool isOnFirstSystem, bool invertMovement)
{
	this->GenericMotionCorrectionFilter::addAssemblyToMove(assembly, invertMovement);
	AssemblyIsOnFirstSystem.push_back(isOnFirstSystem);
}

int BiplaneAlgorithm::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
	info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
	return 1;
}

int BiplaneAlgorithm::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
	info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
	return 1;
}

void BiplaneAlgorithm::SetInput(int index, vtkImageData* input)
{
	if(input != NULL) {
		this->SetInputDataObject(index, input);
		//this->SetInputConnection(index, input->GetProducerPort());
	}
	else {
		//this->SetInputConnection(index, 0); // Setting a NULL input removes the connection.
		this->SetInputDataObject(index, 0);
	}
}

int BiplaneAlgorithm::RequestDataObject(
	vtkInformation*, 
	vtkInformationVector** inputVector , 
	vtkInformationVector* outputVector)
{
	vtkInformation* outInfo1 = outputVector->GetInformationObject(0);
	vtkImageData* outputImage1 = vtkImageData::SafeDownCast( outInfo1->Get(vtkDataObject::DATA_OBJECT()) );

	vtkInformation* outInfo2 = outputVector->GetInformationObject(1);
	vtkImageData* outputImage2 = vtkImageData::SafeDownCast( outInfo2->Get(vtkDataObject::DATA_OBJECT()) );

	// generate default output objects, if not already existing
	if(!outputImage1) {
		outputImage1 = vtkImageData::New();
		//outputImage1->SetPipelineInformation(outInfo1);
		outputImage1->Delete();
	}
	if(!outputImage2) {
		outputImage2 = vtkImageData::New();
		//outputImage2->SetPipelineInformation(outInfo2);
		outputImage2->Delete();
	}

	return 1;
}

int BiplaneAlgorithm::RequestInformation(
	vtkInformation* request,
	vtkInformationVector** inputVector,
	vtkInformationVector* outputVector)
{
	vtkInformation* outInfoImage = outputVector->GetInformationObject(0);
	outInfoImage->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
	
	vtkInformation* outInfoTransform = outputVector->GetInformationObject(1);
	outInfoTransform->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
	
	return 1;
}

int BiplaneAlgorithm::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),	vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
	// request whole image
	vtkInformation* inputInfo1 = inputVector[0]->GetInformationObject(0);
	inputInfo1->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

	vtkInformation* inputInfo2 = inputVector[1]->GetInformationObject(0);
	inputInfo2->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

	return 1;
}

int BiplaneAlgorithm::RequestData(
	vtkInformation* request,
	vtkInformationVector** inputVector,
	vtkInformationVector* outputVector)
{
	/*vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
	vtkDataObject* copy = input->NewInstance();
	copy->ShallowCopy(input);
	this->InternalFilter->SetInput(copy);
	this->InternalFilter->Update();


	vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
	this->InternalFilter->SetInputData(input);
	this->InternalFilter->Update();*/


	vtkInformation* outInfo1 = outputVector->GetInformationObject(0);
	vtkImageData* outputImage1 = vtkImageData::SafeDownCast( outInfo1->Get(vtkDataObject::DATA_OBJECT()) );

	vtkInformation* outInfo2 = outputVector->GetInformationObject(1);
	vtkImageData* outputImage2 = vtkImageData::SafeDownCast( outInfo2->Get(vtkDataObject::DATA_OBJECT()) );

	vtkInformation *inInfo1 = inputVector[0]->GetInformationObject(0);
	vtkImageData* inputImage1 = vtkImageData::SafeDownCast( inInfo1->Get(vtkDataObject::DATA_OBJECT()) );

	vtkInformation *inInfo2 = inputVector[1]->GetInformationObject(0);
	vtkImageData* inputImage2 = vtkImageData::SafeDownCast( inInfo2->Get(vtkDataObject::DATA_OBJECT()) );

	// just pass the images
	//outputImage1->ShallowCopy(inputImage1);	//this will be needed to work with Live Dicoms
	//outputImage2->ShallowCopy(inputImage2);
	//[TODO] ersetzen durch:
	outputImage1->DeepCopy(inputImage1); //danach mit inputImage weiterrechnen und 
	outputImage2->DeepCopy(inputImage2); //outputImage fuer Zeichnen und Ausgabe benutzen
	////
	vector<double> detectedPositionsX, detectedPositionsY, detectedPositionsZ;
	// sets the last parameters
	double motion[3] = {0,0,0};
	ComputeOnBiplaneImages(inputImage1, inputImage2, outputImage1, outputImage2, motion, detectedPositionsX, detectedPositionsY, detectedPositionsZ);

	MotionX = motion[0];
	MotionY = motion[1];
	MotionZ = motion[2];
	Apply3dMotion();

	// projects motion to 2D and apply it
	Apply2dMotion();

	assert(detectedPositionsX.size() == detectedPositionsY.size());
	assert(detectedPositionsY.size() == detectedPositionsZ.size());
	assert(transformations.size() >= detectedPositionsX.size());

	for(unsigned int i = 0; i < detectedPositionsX.size(); ++i) {
		transformations[i]->Identity();
		transformations[i]->Translate(detectedPositionsX[i], detectedPositionsY[i], detectedPositionsZ[i]);
		transformations[i]->Modified();
	}

	return 1;
}

void BiplaneAlgorithm::Apply2dMotion()
{
	double motion[3] = {MotionX, MotionY, MotionZ};

	// apply shift and scale
	motion[0] += ShiftX;
	motion[1] += ShiftY;
	motion[2] += ShiftZ;

	if(UseManualScaling) {
		motion[0] *= ScaleX;
		motion[1] *= ScaleY;
		motion[2] *= ScaleZ;
	}

	
	// apply motion vectors
	for(unsigned int i = 0; i < AssembliesToMove.size(); ++i) {
		// inverse movement only in 3D motion correction
		if(InvertAssemblyMovement[i]) continue;

		vtkMatrix4x4* matrix = AssembliesToMove[i]->GetUserMatrix();
		if(!matrix) {
			cout << __FILE__ << " " << __LINE__ << ": no user matrix!" << endl;
			continue;
		}

		// set translation components in user matrix
		// to the null position + the measured movement
		// 2D motion is only applied to not inverted motion correction
		if(MoveHorizontally) matrix->SetElement(0, 3, AssemblyNullPositionsX[i] + motion[0]);
		if(MoveVertically) matrix->SetElement(1, 3, AssemblyNullPositionsY[i] + motion[1]);

		if(MoveAlongZAxis) matrix->SetElement(2, 3, AssemblyNullPositionsZ[i] + motion[2]);

		AssembliesToMove[i]->Modified();
	}
}

void BiplaneAlgorithm::ComputeOnBiplaneImages(vtkImageData* image1, vtkImageData* image2, vtkImageData* image1forDraw, vtkImageData* image2forDraw, double detectedMotion[3], vector<double>& detectedPositionsX, vector<double>& detectedPositionsY, vector<double>& detectedPositionsZ)
{
	// no motion in the default implementation
	detectedMotion[0] = detectedMotion[1] = detectedMotion[2] = 0.0;
}

vector<AbstractFilterWidget*> BiplaneAlgorithm::getPropertiesGUI()
{
	return this->GenericMotionCorrectionFilter::getPropertiesGUI();
}

// multi map with result points corresponding to the corrLines
vector<cv::Point2d> BiplaneAlgorithm::getEpiLinePoints(vector<cv::Vec3f> corrLines,int imageNumber)
{
	// points for drawing the epiline corresponding to corrLine
	vector<cv::Point2d> corrLinePoints;
	double a, b, c;
	double xValue, yValue;

	a = corrLines[0].val[0];
	b = corrLines[0].val[1];
	c = corrLines[0].val[2];
	// ax+by+c = 0 -> y = (-ax-c)/b
	xValue = 100.0;
	// calculate yValue
	yValue= (-a*xValue-c)/b;
	if (imageNumber == 0){
		biplaneGeometry->firstSystem.transformFromDetectorToImageCoordinates(xValue, yValue);
	}
	if (imageNumber == 1){
		biplaneGeometry->secondSystem.transformFromDetectorToImageCoordinates(xValue, yValue);
	}
	corrLinePoints.push_back(cv::Point2d(xValue, yValue));
	// ax+by+c = 0 -> y = (-ax-c)/b
	xValue = -100.0;
	// calculate yValue
	yValue = (-a*xValue-c)/b;
	if (imageNumber == 0){
		biplaneGeometry->firstSystem.transformFromDetectorToImageCoordinates(xValue, yValue);
	}
	if (imageNumber == 1){
		biplaneGeometry->secondSystem.transformFromDetectorToImageCoordinates(xValue, yValue);
	}
	// store result in multi map
	corrLinePoints.push_back(cv::Point2d(xValue, yValue));
	
	return corrLinePoints;
}
