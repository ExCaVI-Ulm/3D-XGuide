#ifndef MOTION_CORRECTION_FILTER_H
#define MOTION_CORRECTION_FILTER_H

#include <GenericMotionCorrectionFilter.h>
class vtkImageData;
class vtkAssembly;

#include <vector>
//using namespace std;

class AbstractFilterWidget;
class XRayGeometry;

/** Generic filter for motion correction.
    This class expects a vtkImage Data unsigned char image as input and passes
	the image unmodified.
	Motion detected on the image is applied to the assemblies set via the addAssemblyToMove
	method.
	
	Additionally, extra scale and shift can be applied to the motion correction.
 */
class MotionCorrectionFilter : public GenericMotionCorrectionFilter
{
public:
	static MotionCorrectionFilter* New() { return new MotionCorrectionFilter(); }

	void SetInput(vtkImageData* obj) { this->SetInput(0, obj); }
	void SetInput(int index, vtkImageData* obj);

	/// return the input image
	vtkImageData* GetImageOutput();
	
	void setGeometry(XRayGeometry* g);

	virtual std::vector<AbstractFilterWidget*> getPropertiesGUI();
	bool setReferenceFrame;
	bool resetReferenceFrame;

protected:
	MotionCorrectionFilter();
	virtual ~MotionCorrectionFilter();

	/// Let your algorithm operate here.
	/// Motion (as a std::vector from the null position to the current position) is measured in image coordinates.
	virtual void ComputeMotion(vtkImageData* image, double& resultingMotionX, double& resultingMotionY/*, std::vector<double>& resultingPositionsX, std::vector<double>& resultingPositionsY*/);

	/// Handle the VTK pipeline stuff. Do not change in your subclass.
	virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector);	
	virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector);	
	virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
	virtual int RequestDataObject(vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector);
	virtual int FillOutputPortInformation(int port, vtkInformation* info);
	virtual int FillInputPortInformation(int port, vtkInformation* info);

	virtual void Apply2dMotion();

	XRayGeometry* geometry;
	vtkImageData* referenceImage;
	vtkImageData* referenceImageNew;

private:
	ofstream f;
};

#endif // MOTION_CORRECTION_FILTER_H