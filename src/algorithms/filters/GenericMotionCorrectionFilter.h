#ifndef GENERIC_MOTION_CORRECTION_FILTER_H
#define GENERIC_MOTION_CORRECTION_FILTER_H

#include <vtkAlgorithm.h>
#include <vtkImageData.h>

class vtkAssembly;
class vtkTransform;

#include <vector>
//using namespace std;

class AbstractFilterWidget;
class DICOMVisualizer;

/** Generic filter for motion correction.
    This class holds all parts for motion correction which are independent of
	the number of images they operate on.
	Motion detected on the image is applied to the assemblies set via the addAssemblyToMove
	method.
	Scale and shift can be applied to the motion correction.
 */
class GenericMotionCorrectionFilter : public vtkAlgorithm
{
public:
	static GenericMotionCorrectionFilter* New() { return new GenericMotionCorrectionFilter(); }

	/// get/set whether to update the position of overlays in the respective direction
	vtkGetMacro(MoveHorizontally, bool);
	vtkSetMacro(MoveHorizontally, bool);
	vtkGetMacro(MoveVertically, bool);
	vtkSetMacro(MoveVertically, bool);
	vtkGetMacro(MoveAlongZAxis, bool);
	vtkSetMacro(MoveAlongZAxis, bool);

	/// Get the current movement.
	vtkGetMacro(MotionX, double);
	vtkGetMacro(MotionY, double);
	vtkGetMacro(MotionZ, double);

	/// get/set the movement's scale and shift
	vtkGetMacro(UseManualScaling, bool);
	vtkSetMacro(UseManualScaling, bool);
	vtkGetMacro(ScaleX, double);
	vtkSetMacro(ScaleX, double);
	vtkGetMacro(ScaleY, double);
	vtkSetMacro(ScaleY, double);
	vtkGetMacro(ScaleZ, double);
	vtkSetMacro(ScaleZ, double);
	vtkGetMacro(ShiftX, double);
	vtkSetMacro(ShiftX, double);
	vtkGetMacro(ShiftY, double);
	vtkSetMacro(ShiftY, double);
	vtkGetMacro(ShiftZ, double);
	vtkSetMacro(ShiftZ, double);

	/// get/set the pixel to mm scaling
	vtkGetMacro(ImageToWorldCoordinatesScaling, double);
	vtkSetMacro(ImageToWorldCoordinatesScaling, double);

	/// set/get wheter the filter actually calls it's algorithm
	vtkGetMacro(ProcessImages, bool);
	vtkSetMacro(ProcessImages, bool);

	vtkGetMacro(UseUnsharpMasking, bool);
	vtkSetMacro(UseUnsharpMasking, bool);
	vtkGetMacro(MaskingSigma, double);
	vtkSetMacro(MaskingSigma, double);
	vtkGetMacro(MaskingWeight, double);
	vtkSetMacro(MaskingWeight, double);
	vtkGetMacro(UseSobelFilter, bool);
	vtkSetMacro(UseSobelFilter, bool);
	vtkGetMacro(UseCensus, bool);
	vtkSetMacro(UseCensus, bool);
	vtkGetMacro(KSizeSobel, int);
	vtkSetMacro(KSizeSobel, int);
	vtkGetMacro(ScaleSobel, int);
	vtkSetMacro(ScaleSobel, int);
	vtkGetMacro(DeltaSobel, int);
	vtkSetMacro(DeltaSobel, int);
	vtkGetMacro(UseScharrFilter, bool);
	vtkSetMacro(UseScharrFilter, bool);
	vtkGetMacro(ScaleScharr, int);
	vtkSetMacro(ScaleScharr, int);
	vtkGetMacro(DeltaScharr, int);
	vtkSetMacro(DeltaScharr, int);

	vtkGetMacro(UseDLManualScaling, bool);
	vtkSetMacro(UseDLManualScaling, bool);
	vtkGetMacro(MoveX, bool);
	vtkSetMacro(MoveX, bool);
	vtkGetMacro(MoveY, bool);
	vtkSetMacro(MoveY, bool);
	vtkGetMacro(DLScaleX, double);
	vtkSetMacro(DLScaleX, double);
	vtkGetMacro(DLScaleY, double);
	vtkSetMacro(DLScaleY, double);

	/// Add an assembly to apply the movement to.
	void addAssemblyToMove(vtkAssembly* assembly, bool invertMovement = false);

	/// Null position := position of the assemblies in the frame where the template is selected.
	void setCurrentAssemblyPositionAsNullPositions();

	/// Methods to access the tracked positions
	std::vector<vtkTransform*> setNumberOfTransformations(unsigned int nr);
	std::vector<vtkTransform*> getTransforms() const;

	/// Methods regarding the DICOMVisualizer
	void setVolumeVisualizerToUpdate(DICOMVisualizer* viewer);
	void setTrackingPointIndexToUseForVolumeVisualizer(int index);


	/// Returns new GUI for this filter.
	/// For your filter, append your GUI to the superclass's return value!
	virtual std::vector<AbstractFilterWidget*> getPropertiesGUI();

	/// Dispatch the VTK pipeline stuff.
	virtual int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

protected:
	GenericMotionCorrectionFilter();
	virtual ~GenericMotionCorrectionFilter();

	/// Handle the VTK pipeline stuff.
	virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector);	
	virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector);	
	virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
	virtual int RequestDataObject(vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector);

	/// Apply the motion set in MotionX, MotionY (and MotionZ)
	virtual void Apply3dMotion();
	virtual void Apply2dMotion();

	bool ProcessImages;	/// Wheter the algorithm computes (true) or just passes the images in the pipeline (false)

	bool MoveVertically, MoveHorizontally, MoveAlongZAxis; /// Whether to update the overlay in the respective direction.
	bool MoveX, MoveY;
	double DLScaleX, DLScaleY;
	/// Scale and shift applied to the movement.
	bool UseManualScaling; // whether to use the user defined scaling or automatic computation for each assembly
	bool UseDLManualScaling;
	double ScaleX, ScaleY, ScaleZ;
	double ShiftX, ShiftY, ShiftZ; 
	bool UseUnsharpMasking, UseSobelFilter, UseScharrFilter, UseCensus;
	double MaskingSigma, MaskingWeight;
	int KSizeSobel, ScaleSobel, DeltaSobel, ScaleScharr, DeltaScharr;
	
	double ImageToWorldCoordinatesScaling; /// to transform the motion

	typedef std::vector<vtkAssembly*> AssemblyVector;
	AssemblyVector AssembliesToMove;

	std::vector<bool> InvertAssemblyMovement;

	std::vector<double> AssemblyNullPositionsX; /// Position of the assemblies in the frame where the template is selected.
	std::vector<double> AssemblyNullPositionsY; /// Position of the assemblies in the frame where the template is selected.
	std::vector<double> AssemblyNullPositionsZ; /// Position of the assemblies in the frame where the template is selected.

	std::vector<vtkTransform*> transformations; /// To store the tracking results.

	DICOMVisualizer* volumeVisualizer;
	int trackingPointToUseForVolumeVisualizerUpdate;


	/// Set this variables in your subclass to the current motion.
	double MotionX;
	double MotionY;
	double MotionZ;

private:
};

#endif // GENERIC_MOTION_CORRECTION_FILTER_H