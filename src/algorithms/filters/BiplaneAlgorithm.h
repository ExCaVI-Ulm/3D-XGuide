#ifndef BIPLANE_ALGORITHM_H
#define BIPLANE_ALGORITHM_H

#include <GenericMotionCorrectionFilter.h>
class vtkImageData;
class BiplaneGeometry;
class vtkTransform;
#include <cv.h>
#include <vector>
//using namespace std;

/** Filter operating in a biplane system with known geometry.
  * It gets two images as input. The 3D detected position can be accessed via
  * the getTransform() method.
  */
class BiplaneAlgorithm : public GenericMotionCorrectionFilter
{
public:
	static BiplaneAlgorithm* New() { return new BiplaneAlgorithm(); }

	/// index is 0 or 1, for the respective channel
	void SetInput(int index, vtkImageData* obj);
	void SetInputImage1(vtkImageData* obj) { SetInput(0, obj); }
	void SetInputImage2(vtkImageData* obj) { SetInput(1, obj); }

	vtkImageData* GetOutputImage(int idx) { return vtkImageData::SafeDownCast(this->GetOutputDataObject(idx)); }
	vtkImageData* GetOutputImage1() { return vtkImageData::SafeDownCast(this->GetOutputDataObject(0)); }
	vtkImageData* GetOutputImage2() { return vtkImageData::SafeDownCast(this->GetOutputDataObject(1)); }

	void setBiplaneGeometry(const BiplaneGeometry* geom);
	const BiplaneGeometry* getBiplaneGeometry() const;

	virtual std::vector<AbstractFilterWidget*> getPropertiesGUI();

	void addAssemblyToMove(vtkAssembly* assembly, bool isOnFirstSystem, bool invertMovement = false);

protected:
	BiplaneAlgorithm();
	virtual ~BiplaneAlgorithm();

	// multi map with result points corresponding to the corrLines
	std::vector<cv::Point2d> getEpiLinePoints(std::vector<cv::Vec3f> corrLines, int imageNumber);

	/// Let your algorithm operate here.
	virtual void ComputeOnBiplaneImages(vtkImageData* image1, vtkImageData* image2, vtkImageData* image1forDraw, vtkImageData* image2forDraw, double detectedMotion[3], std::vector<double>& detectedPositionsX, std::vector<double>& detectedPositionsY, std::vector<double>& detectedPositionsZ);

	const BiplaneGeometry* biplaneGeometry;
	std::vector<bool> AssemblyIsOnFirstSystem; // whether to use first or second geometry
	
	virtual void Apply2dMotion(); // overloaded to distinguish between left and right system

	/// Handle the VTK pipeline stuff. Do not change in your subclass.
	virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector);	
	virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector);	
	virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
	virtual int RequestDataObject(vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector);
	virtual int FillOutputPortInformation(int port, vtkInformation* info);
	virtual int FillInputPortInformation(int port, vtkInformation* info);

	std::vector< std::vector< cv::Vec3f > > encEpipolarLines; // encoded epipolar lines

private:
};

#endif // BIPLANE_ALGORITHM_H