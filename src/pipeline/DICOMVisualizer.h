#ifndef DICOMVISUALIZER_H_
#define DICOMVISUALIZER_H_

//#ifdef COMPILE_WITH_GDCM

// VTK includes
#include <vtkGdcmImageReader.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkImageReader2.h>

// VTK classes
class vtkImageShiftScale;
class vtkImageReslice;
class vtkImageActor;
class vtkAssembly;
class vtkImageMagnify;
class vtkImageChangeInformation;

#include <stdlib.h>
#include <string>

//using namespace std;

class DICOMVisualizer
{
public:
	
	DICOMVisualizer(vtkAssembly* assembly);
	~DICOMVisualizer();

	// -- connect through vtkGdcmImageReader methods
	//vtkImageData* GetOutput() { return theDICOMReader->GetOutput(); };
	//vtkAlgorithmOutput* GetOutputPort() { return theDICOMReader->GetOutputPort(); };
		
	const char* getInputFile() const;
	void setMRInputFile(std::string theFilename);
	void setCTInputFile(std::string theFilename, int orientation);
	void getVolumeBounds(double bounds[6]) { return theDICOMReader->GetOutput()->GetBounds(bounds); };	// get bounds for X,Y and Z
	//void getRescaledVolumeBounds(double zSpacing, double zThickness);
	void rescaleCTVolume(std::string theFilename);
	double getSliceSpacing();
	void setSliceSpacing(double spacing);

	void getRegisteredVolumeBounds(double bounds[6]);
	void getImagePositionPatient(double reg[3]);	// According to the VTK structured point file MESH, the mesh origin is the same as the PAtientPosition in respective DICOM header.
															// Thus this function get the volume's position in world coordinates.
	void setCTVolumeRegistration();
	void setMRVolumeRegistration();

	void SetLevelWindow(double scaling_factor); // can be a value from 0.0 to 255.0
	double GetLevelWindow();	
	void setVisibility(bool visible);

	void setCTSliceIntersectionPoint(double pos[3]);
	void setCTSliceIntersectionPoint(double x, double y, double z);
	/// Set/Get the position of the slices intersection point in local coordinates.
	void setCTSliceIntersectionPointLocal(double localPos[3]);
	void setMRSliceIntersectionPoint(double localPos[3]);
	void getSliceIntersectionPointLocal(double localPos[3]);

	void SetDICOMRenderer3D(vtkRenderer* renderer);
	void SetDICOMRenderers2D(vtkRenderer* renderers[3]);
	vtkRenderer* const GetDICOMRenderer3D() { return dicomRenderer3D; };
	vtkRenderer* GetDICOMRenderers2D(unsigned int number) const; //o, 1, 2 for respectively X, Y, Z planes of MR images
	double getSliceSpacingFromDICOM() { return slice_spacing; };

private:

	void Update() { theDICOMReader->Update(); };
	void SetFileName(std::string theFilename) { theDICOMReader->SetFileName(theFilename.c_str()); };
	
	vtkAssembly* assembly;
	vtkGDCMImageReader* theDICOMReader;	
	vtkImageShiftScale* shifter;
	vtkSmartPointer<vtkImageReslice> reslices[3];

	vtkSmartPointer<vtkImageActor> actorX;
	vtkSmartPointer<vtkImageActor> actorY;
	vtkSmartPointer<vtkImageActor> actorZ;

	vtkSmartPointer<vtkImageActor> actors[3];
	vtkSmartPointer<vtkRenderer> dicomRenderers2D[3];
	vtkSmartPointer<vtkRenderer> dicomRenderer3D;
	
	double* range;
	double levelWindow;
	double slice_spacing;
	double slice_spacing_for_registration;
	double slice_thickness;
	bool CTsequence;
	bool PhilipsMesh;
	double volumePosition[3]; // registration
	double localIntersectionPosition[3];
	bool spacing_set;

	vtkSmartPointer<vtkImageMagnify> magnifyFilter;
	vtkSmartPointer<vtkImageChangeInformation> changeFilter;

};

//#endif /* COMPILE_WITH_GDCM */

#endif /* DICOMVISUALIZER_H_ */

