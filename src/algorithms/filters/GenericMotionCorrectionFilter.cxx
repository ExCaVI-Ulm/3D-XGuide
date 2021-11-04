#include "GenericMotionCorrectionFilter.h"

#include <vtkInformation.h>
#include <vtkDemandDrivenPipeline.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkInformationVector.h>
#include <vtkAssembly.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>

#include "MotionCorrectionFilterWidget.h"
#include "PreprocessingFilterWidget.h"
#include "DICOMVisualizer.h"

#include <iostream>
using namespace std;

GenericMotionCorrectionFilter::GenericMotionCorrectionFilter()
: ProcessImages(true),
MoveVertically(true), MoveHorizontally(true), MoveAlongZAxis(true), 
UseManualScaling(false),
ScaleX(1.0), ScaleY(1.0), ScaleZ(1.0),
ShiftX(0.0), ShiftY(0.0), ShiftZ(0.0),
ImageToWorldCoordinatesScaling(1.0),
MotionX(0), MotionY(0), MotionZ(0),
UseUnsharpMasking(false), MaskingSigma(1.0), MaskingWeight(1.0),
UseSobelFilter(false), KSizeSobel(3), ScaleSobel(2), DeltaSobel(0),
UseScharrFilter(false), ScaleScharr(1), DeltaScharr(0),
UseCensus(false), UseDLManualScaling(false), MoveX(false), MoveY(true), DLScaleX(1.0), DLScaleY(1.0),
volumeVisualizer(0), trackingPointToUseForVolumeVisualizerUpdate(-1)
{
}

GenericMotionCorrectionFilter::~GenericMotionCorrectionFilter()
{
}

int GenericMotionCorrectionFilter::ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
	// generate the data
	if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA())) {
		return this->RequestData(request, inputVector, outputVector);
	}

	// extent information
	if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT())) 
	{
		return this->RequestUpdateExtent(request, inputVector, outputVector);
	}

	// create the output
	if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT())) {
		return this->RequestDataObject(request, inputVector, outputVector);
	}

	// execute information
	if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION())) {
		return this->RequestInformation(request, inputVector, outputVector); 
	}

	// else, let superclass handle this
	return this->vtkAlgorithm::ProcessRequest(request, inputVector, outputVector);
}

int GenericMotionCorrectionFilter::RequestDataObject(
	vtkInformation*, 
	vtkInformationVector** inputVector , 
	vtkInformationVector* outputVector)
{
	return 1;
}

int GenericMotionCorrectionFilter::RequestInformation(
	vtkInformation* request,
	vtkInformationVector** inputVector,
	vtkInformationVector* outputVector)
{
	return 1;
}

int GenericMotionCorrectionFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),	vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
	return 1;
}

int GenericMotionCorrectionFilter::RequestData(
	vtkInformation* request,
	vtkInformationVector** inputVector,
	vtkInformationVector* outputVector)
{
	return 1;
}

void GenericMotionCorrectionFilter::addAssemblyToMove(vtkAssembly* assembly, bool invertMovement)
{
	// add assembly and default "null position"
	AssembliesToMove.push_back(assembly);

	AssemblyNullPositionsX.push_back(assembly->GetPosition()[0]); 
	AssemblyNullPositionsY.push_back(assembly->GetPosition()[1]); 
	AssemblyNullPositionsZ.push_back(assembly->GetPosition()[2]); 

	//invertMovement=true for 3D
	InvertAssemblyMovement.push_back(invertMovement);
}

void GenericMotionCorrectionFilter::setCurrentAssemblyPositionAsNullPositions()
{
	AssemblyNullPositionsX.resize(AssembliesToMove.size());
	AssemblyNullPositionsY.resize(AssembliesToMove.size());
	AssemblyNullPositionsZ.resize(AssembliesToMove.size());

	for(unsigned int i = 0; i < AssembliesToMove.size(); ++i) {
		vtkMatrix4x4* matrix = AssembliesToMove[i]->GetUserMatrix();

		if(!matrix) {
			// ensure that every assembly has a user matrix
			matrix = vtkMatrix4x4::New();
			AssembliesToMove[i]->SetUserMatrix(matrix);
			matrix->Delete();
		}

		AssemblyNullPositionsX[i] = matrix->GetElement(0, 3);
		AssemblyNullPositionsY[i] = matrix->GetElement(1, 3);
		AssemblyNullPositionsZ[i] = matrix->GetElement(2, 3);


	}


}

void GenericMotionCorrectionFilter::setVolumeVisualizerToUpdate(DICOMVisualizer* viewer)
{
	volumeVisualizer = viewer;
}

void GenericMotionCorrectionFilter::setTrackingPointIndexToUseForVolumeVisualizer(int index)
{
	trackingPointToUseForVolumeVisualizerUpdate = index;
}

void GenericMotionCorrectionFilter::Apply2dMotion()
{
	// apply shift and scale
	double motionX = MotionX + ShiftX;
	double motionY = MotionY + ShiftY;

	if(UseManualScaling) {
		motionX *= ScaleX;
		motionY *= ScaleY;
	}

	// transform from image to world coordinates
	motionX *= ImageToWorldCoordinatesScaling;
	motionY *= ImageToWorldCoordinatesScaling;

	// apply motion vectors
	for(unsigned int i = 0; i < AssembliesToMove.size(); ++i) {
		vtkMatrix4x4* matrix = AssembliesToMove[i]->GetUserMatrix();
		if(!matrix) {
			cout << __FILE__ << " " << __LINE__ << ": no user matrix!" << endl;
			continue;
		}

		// set translation components in user matrix
		// to the null position + the measured movement
		// 2D motion is only applied to not inverted motion correction
		if(!InvertAssemblyMovement[i]) {
			if(MoveHorizontally) matrix->SetElement(0, 3, AssemblyNullPositionsX[i] + motionX);
			if(MoveVertically) matrix->SetElement(1, 3, AssemblyNullPositionsY[i] + motionY);
	
			AssembliesToMove[i]->Modified();
		}

	}
}

void GenericMotionCorrectionFilter::Apply3dMotion()
{
	// apply shift and scale
	double motionX = MotionX + ShiftX;
	double motionY = MotionY + ShiftY;
	double motionZ = MotionZ + ShiftZ;

	motionX *= ScaleX;
	motionY *= ScaleY;
	motionZ *= ScaleZ;

	// apply motion vectors
	for(unsigned int i = 0; i < AssembliesToMove.size(); ++i) {
		vtkMatrix4x4* matrix = AssembliesToMove[i]->GetUserMatrix();
		if(!matrix) {
			cout << __FILE__ << " " << __LINE__ << ": no user matrix!" << endl;
			continue;
		}

		// set translation components in user matrix
		// to the null position - the measured movement
		// 3D motion is only applied to inverted motion correction
		if(InvertAssemblyMovement[i]) {
			if(MoveHorizontally) matrix->SetElement(0, 3, AssemblyNullPositionsX[i] - motionX);
			if(MoveVertically) matrix->SetElement(1, 3, AssemblyNullPositionsY[i] - motionY);
			if(MoveAlongZAxis) matrix->SetElement(2, 3, AssemblyNullPositionsZ[i] - motionZ);

			AssembliesToMove[i]->Modified();
		}
	}

	// take care of the VolumeOrhtoViewer update
	if(!volumeVisualizer) return;
	if(trackingPointToUseForVolumeVisualizerUpdate >= transformations.size()) return;
	if(trackingPointToUseForVolumeVisualizerUpdate < 0) return;

	vtkMatrix4x4* m = transformations[trackingPointToUseForVolumeVisualizerUpdate]->GetMatrix();
	// apply inverse motion correction, because the volume is displayed in 3D window
	
	volumeVisualizer->setCTSliceIntersectionPoint(
		m->Element[0][3] - motionX,
		m->Element[1][3] - motionY,
		m->Element[2][3] - motionZ
	);
}

vector<vtkTransform*> GenericMotionCorrectionFilter::setNumberOfTransformations(unsigned int nr)
{
	if(nr != transformations.size()) {
		// delete the old ones
		for(unsigned int i = 0; i < transformations.size(); ++i) {
			transformations[i]->Delete();
		}

		transformations.clear();

		// create new transformations
		for(unsigned int i = 0; i < nr; ++i) {
			transformations.push_back(vtkTransform::New());
		}

		Modified(); // trigger update
	}

	return transformations;
}

vector<vtkTransform*> GenericMotionCorrectionFilter::getTransforms() const
{
	return transformations;
}

vector<AbstractFilterWidget*> GenericMotionCorrectionFilter::getPropertiesGUI()
{
	vector<AbstractFilterWidget*> res;
	res.push_back(new MotionCorrectionFilterWidget(this));
	res.push_back(new PreprocessingFilterWidget(this));

	return res;
}