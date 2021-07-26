//#ifdef COMPILE_WITH_GDCM

#include "DICOMVisualizer.h"

#include <vtkImageShiftScale.h>
#include <vtkImageReslice.h>
#include <vtkImageActor.h>
#include <vtkAssembly.h>
#include <vtkStructuredPointsReader.h>
#include <vtkStructuredPoints.h>
#include <vtkMatrix4x4.h>
#include <vtkImageMagnify.h>
#include <vtkImageChangeInformation.h>
#include <vtkProp3DCollection.h>

#include <gdcmReader.h>
#include <gdcmFile.h>
#include <gdcmTag.h>
#include <gdcmStringFilter.h>

#include <gdcmDirectory.h>
#include <gdcmIPPSorter.h>
#include <vtkStringArray.h>

#include <stdlib.h>

using namespace std;

DICOMVisualizer::DICOMVisualizer(vtkAssembly* assembly)
{

	theDICOMReader = vtkGDCMImageReader::New();	
	

	// In the CT .dcm file exported by the ITK-SNAP V.2 the z-spacing is reduced, thus we need
	// vtkImageChangeInformation for modifying the spacing (origin, extent) of the data without changing the data itself
	changeFilter = vtkSmartPointer<vtkImageChangeInformation>::New();
	changeFilter->SetInputConnection(theDICOMReader->GetOutputPort());

	shifter = vtkImageShiftScale::New();
	shifter->SetOutputScalarTypeToUnsignedChar();
	shifter->ReleaseDataFlagOff();
	shifter->SetInputConnection(changeFilter->GetOutputPort());


	this->assembly = assembly;
	slice_spacing = -1.0;
	spacing_set = false;

	// defines along which axis of the MR volume the slice will read
	// assumes volume in "RAI" coordinates
	// x-axis should be mirrored to get the volume displayed the same way as the mesh
	const double sliceOrientations[3][9] = {
		{ 0,1,0, 0,0,1, 1,0,0 },  // x-axis = yz-plane
		{-1,0,0, 0,0,1, 0,1,0 }, // y-axis = xz-plane
		{-1,0,0, 0,1,0, 0,0,1 }  // z-axis = xy-plane		
	};


	for (int i = 0; i < 3; ++i) {

		reslices[i] = vtkImageReslice::New();
		reslices[i]->SetInputConnection(shifter->GetOutputPort());
		reslices[i]->SetOutputDimensionality(2);
		reslices[i]->SetResliceAxesDirectionCosines(sliceOrientations[i]);

		actors[i] = vtkSmartPointer<vtkImageActor>::New();	
		
	}

	actorX = vtkSmartPointer<vtkImageActor>::New();
	actorX->RotateZ(90.0);
	actorX->RotateX(90.0);
	actorY = vtkSmartPointer<vtkImageActor>::New();
	actorY->RotateX(90.0);
	actorZ = vtkSmartPointer<vtkImageActor>::New();

	assembly->AddPart(actorX);
	assembly->AddPart(actorY);
	assembly->AddPart(actorZ);

}

DICOMVisualizer::~DICOMVisualizer()
{
	theDICOMReader->Delete();
	shifter->Delete();
	changeFilter->Delete();

	for (int i = 0; i < 3; ++i) {
		actors[i]->Delete();
		reslices[i]->Delete();
	}
	
}

const char* DICOMVisualizer::getInputFile() const
{
	const char* filename = theDICOMReader->GetFileName();
	
	if (filename == NULL)
	{
		vtkStringArray *filenames = theDICOMReader->GetFileNames();
		filename = filenames->GetValue(0).c_str();

	}

	return filename;
}


void DICOMVisualizer::setMRInputFile(string theFilename)
{
	CTsequence = false;
	theDICOMReader->SetFileName(theFilename.c_str());
	theDICOMReader->Update();
	
	theDICOMReader->Print(cout);

	levelWindow = 255.0;
	
	range = theDICOMReader->GetOutput()->GetScalarRange();
	//double contrast = 10;
	//shifter->SetShift(range[0]); // level
	//shifter->SetScale(levelWindow*contrast / (range[1] - range[0])); // window

	double contrast = 1;
	shifter->SetShift(-range[1] / contrast);	// level
	shifter->SetScale((levelWindow / contrast) / ((range[1] - range[0]) / contrast));
	shifter->Update();

	for (int i = 0; i < 3; ++i) {
		actors[i]->SetInputData(reslices[i]->GetOutput());
		reslices[i]->Update();
		dicomRenderers2D[i]->AddActor(actors[i]);

	}

	actorX->SetInputData(reslices[0]->GetOutput());
	actorY->SetInputData(reslices[1]->GetOutput());
	actorZ->SetInputData(reslices[2]->GetOutput());

	setMRVolumeRegistration();

}

void DICOMVisualizer::rescaleCTVolume(string theFilename)
{
	gdcm::Reader* reader = new gdcm::Reader;
	//const char * filename = theFilename.c_str();
	reader->SetFileName(theFilename.c_str());

	if (!reader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve primary angle information from file!" << std::endl;
	}

	bool success = reader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
	}

	gdcm::File dicomFile = reader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;
	dicomStringFilter->SetFile(dicomFile);	

	if (!spacing_set)
	{
		gdcm::Tag tag = gdcm::Tag(0x0018, 0x0088);
		//gdcm::Tag tag = gdcm::Tag(0x0018, 0x0050);
		string strSpacing = dicomStringFilter->ToString(tag);
		// for patients PHILIPS system
		slice_spacing = atof(strSpacing.c_str());

		gdcm::Tag tag2 = gdcm::Tag(0x0018, 0x0050);
		string strThickness = dicomStringFilter->ToString(tag2);
		slice_thickness = atof(strThickness.c_str());

		if (slice_spacing == 0.0)
		{
			slice_spacing = slice_thickness;
			//TODO: replace slice_thickness with [Slice_location(last) - Slice_location(first)] / SliceExtent

		}
	}
	else
		spacing_set = false;

	delete(reader);

}

double DICOMVisualizer::getSliceSpacing()
{
	return slice_spacing;
}


void DICOMVisualizer::setSliceSpacing(double spacing)
{
	slice_spacing = spacing;
	spacing_set = true;
}


void DICOMVisualizer::setCTInputFile(string theFilename, string theFoldername, int orientation)
{
	CTsequence = true;

	gdcm::Directory d;
	d.Load(theFoldername.c_str(), false);

	const std::vector<std::string> & filenames = d.GetFilenames();

	gdcm::IPPSorter s;

	s.SetComputeZSpacing(true);

	s.SetZSpacingTolerance(1e-3);
	//std::cout << "z-spacing" << s.GetZSpacing() << std::endl;

	bool b = s.Sort(filenames);

	if (!b)
	{
		std::cerr << "DICOM volume contains more than one image series: " << "please choose another DICOM series" << std::endl;
		return;
	}


	const std::vector<std::string> & sorted = s.GetFilenames();


	vtkStringArray *files = vtkStringArray::New();

	std::vector< std::string >::const_iterator it = sorted.begin();

	for (; it != sorted.end(); ++it)
	{
		const std::string &f = *it;
		files->InsertNextValue(f.c_str());
	}

	if (sorted.size() > 1)		
		theDICOMReader->SetFileNames(files);
	else
		theDICOMReader->SetFileName(theFilename.c_str());


	theDICOMReader->Update();
	//theDICOMReader->Print(cout);

	rescaleCTVolume(theFilename); // get the slice_spacing/slice thickness from DICOM header
	// we have artificially rescale CT image, thus set new data spacing and new origin for z-coordinate 
	double spacing[3], bounds[6], origin[3];
	theDICOMReader->GetOutput()->GetSpacing(spacing);
	changeFilter->SetOutputSpacing(spacing[0], spacing[1], slice_spacing);

	/*theDICOMReader->GetOutput()->GetOrigin(origin);
	changeFilter->SetOutputOrigin(origin[0], origin[1], origin[2]*slice_spacing);*/
	changeFilter->Update();


	levelWindow = 255.0;
	range = theDICOMReader->GetOutput()->GetScalarRange();
	
	double contrast = 1;
	shifter->SetShift(-range[1] / contrast);	// level
	shifter->SetScale((levelWindow / contrast) / ((range[1] - range[0]) / contrast));

	//this is for MRI
	//double contrast = 10;
	//shifter->SetShift(-range[1]); // level
	//shifter->SetScale(levelWindow*contrast/ (range[1] - range[0])); // window
	//shifter->SetOutputScalarTypeToUnsignedChar();
	//
	shifter->Update();

	//for the PHILIPS MESH y-axis should be mirrored in order mesh is at correct position
	// wrt DICOM set
	const double sliceOrientationsPhilips[3][9] = {
			{ 0,-1,0, 0,0,1, 1,0,0 },  // x-axis = yz-plane
			{ 1,0,0, 0,0,1, 0,1,0 }, // y-axis = xz-plane
			{ 1,0,0, 0,-1,0, 0,0,1 }  // z-axis = xy-plane
		};


	//for the ITK-SNAP MESH x-axis should be mirrored in order mesh is at correct position
	// wrt DICOM set
	
	const double sliceOrientationsITK[3][9] = {
			{ 0,1,0, 0,0,1, 1,0,0 },  // x-axis = yz-plane
			{-1,0,0, 0,0,1, 0,1,0 }, // y-axis = xz-plane
			{ -1,0,0, 0,1,0, 0,0,1 }  // z-axis = xy-plane
		};

	for (int i = 0; i < 3; ++i) {

		if (orientation == 0 || orientation == 2 || orientation == 4)
		{
			PhilipsMesh = true;
			reslices[i]->SetResliceAxesDirectionCosines(sliceOrientationsPhilips[i]);
			if (orientation == 0 || orientation == 4)
			{
				slice_spacing_for_registration = 1.0;
			}
			else
			{
				slice_spacing_for_registration = slice_spacing;
			}
		}	
		
		else if (orientation == 3)
		{
			PhilipsMesh = false;
			reslices[i]->SetResliceAxesDirectionCosines(sliceOrientationsITK[i]);
			slice_spacing_for_registration = slice_spacing;
		}
		//reslices[i]->SetOutputSpacing(spacing[0], spacing[1], slice_spacing);
		//reslices[i]->UpdateInformation();
		actors[i]->SetInputData(reslices[i]->GetOutput());
		reslices[i]->Update();
		dicomRenderers2D[i]->AddActor(actors[i]);
	}


	actorX->SetInputData(reslices[0]->GetOutput());
	actorY->SetInputData(reslices[1]->GetOutput());
	actorZ->SetInputData(reslices[2]->GetOutput());

	setCTVolumeRegistration();

}

void DICOMVisualizer::SetLevelWindow(double scaling_factor)
{

	if (scaling_factor > 255.0) { scaling_factor = 255.0; };
	if (scaling_factor < 0) { scaling_factor = 0; };
	
	levelWindow = scaling_factor;
	double contrast = 1;
	shifter->SetScale(levelWindow*contrast / (range[1] - range[0]));	
	shifter->Update();
	for (int i = 0; i < 3; i++)
	{		
		reslices[i]->Update();
	}

}

double DICOMVisualizer::GetLevelWindow()
{
	return levelWindow;
}

void DICOMVisualizer::setVisibility(bool visible)
{
	/*for (int i = 0; i < 3; ++i) {
		actors[i]->SetVisibility(visible);
	}*/
	actorX->SetVisibility(visible);
	actorY->SetVisibility(visible);
	actorZ->SetVisibility(visible);
}
void DICOMVisualizer::getImagePositionPatient(double reg[3])
{

	double* origin = theDICOMReader->GetImagePositionPatient();

	reg[0] = origin[0];
	reg[1] = origin[1];
	reg[2] = origin[2];

	
}


void DICOMVisualizer::setCTVolumeRegistration()
{
	double reg[3];
	this->getImagePositionPatient(reg);

	double bounds[6];
	this->getVolumeBounds(bounds);

	volumePosition[0] = -bounds[0] - (bounds[1] - bounds[0]) / 2.0;
	volumePosition[1] = -bounds[2] - (bounds[3] - bounds[2]) / 2.0;
	//volumePosition[2] = -bounds[4] - slice_spacing*(bounds[5] - bounds[4]) / 2.0; //for CT
	volumePosition[2] = -bounds[4] - slice_spacing_for_registration*(bounds[5] - bounds[4]) / 2.0; //for MRI

	// Set CT volume coordinate system origin
	double CTorigin[3] = { 0.0, 0.0, 0.0 };
	setCTSliceIntersectionPointLocal(CTorigin);
}

void DICOMVisualizer::setCTSliceIntersectionPoint(double pos[3])
{
	// convert from world to local (volume) coordinate system
	double tmp[4] = { pos[0], pos[1], pos[2], 1 };
	double localPos[4];

	vtkMatrix4x4* m = vtkMatrix4x4::New();
	vtkMatrix4x4::Invert(assembly->GetUserMatrix(), m);
	m->MultiplyPoint(tmp, localPos);
	localPos[0] /= localPos[3];
	localPos[1] /= localPos[3];
	localPos[2] /= localPos[3];
	m->Delete();

	setCTSliceIntersectionPointLocal(localPos);
}

void DICOMVisualizer::setCTSliceIntersectionPoint(double x, double y, double z)
{
	double tmp[3] = { x, y, z };
	setCTSliceIntersectionPoint(tmp);
}

void DICOMVisualizer::setMRVolumeRegistration()
{
	double reg[3];
	this->getImagePositionPatient(reg);

	double bounds[6];
	this->getVolumeBounds(bounds);
	// TRANSLATION to the Patient CS: Set the origin of the MR volume coordinate system (0,0,0) in the DICOM Image Position (Tag 0020x0032 in DICOM header)
	// which is the same as the origin of the vtk MESH exported by ITK-SNAP

	// X and Z coorinates of the registration (according to the "origin" in the vtk structured point file or Image Position in DICOM header)
	// are the same as the lower x and z volume limits of the vtkGdcmImageReader. Thus:
	// The both y-axis limits of the vtkGdcmImageReader (bounds[2] and bounds[3] from vtkGdcmImageReader->GetBounds(bounds) are positive,
	// whereas the y-coordinate of the vtk MESH origin (according to the vtk structured point file) has negative sign.
	// Thus the y-axis is inverted in VTK compared to the MR volume and
	// the local y-value must be computed "from the other side."

	volumePosition[0] = reg[0] + (bounds[1] - bounds[0]) / 2.0;
	volumePosition[1] = -(bounds[3] + reg[1]);
	volumePosition[2] = reg[2] + (bounds[5] - bounds[4]) / 2.0;

	// Set MR volume coordinate system origin at the world (0,0,0)
	// with further convertion from world to local (volume) coordinate system
	double MRoriginWorld[3] = { 0.0, 0.0, 0.0 };
	setMRSliceIntersectionPoint(MRoriginWorld);
}

void DICOMVisualizer::getRegisteredVolumeBounds(double bounds[6])
{
	// bounds of the volume (0..size)
	this->getVolumeBounds(bounds);
	// valid ranges for local point: relative to volume position
	bounds[0] += volumePosition[0];
	bounds[1] += volumePosition[0];
	bounds[2] += volumePosition[1];
	bounds[3] += volumePosition[1];
	if (CTsequence)
	{
		bounds[5] = slice_spacing_for_registration*(bounds[5] - bounds[4]) / 2.0;
		bounds[4] += volumePosition[2];
	}
	else
	{ 
		bounds[4] += volumePosition[2];
		bounds[5] += volumePosition[2];
	}


}

void DICOMVisualizer::setMRSliceIntersectionPoint(double localPos[3])
{
	// Slices origin must be computed relative to the registration.
	double orig[3];
	for (int i = 0; i < 3; ++i) {
		orig[i] = localPos[i] - volumePosition[i];
	}	

	for (int i = 0; i < 3; ++i) {
		reslices[i]->SetResliceAxesOrigin(orig);
		reslices[i]->Update();
	}


	// Because the volume's x-axis is flipped (see sliceOrientations
	// in DICOMVisualizer's constructor), the localPos's x-value
	// must be flipped.
	localPos[0] = -localPos[0];

	// store parameters
	localIntersectionPosition[0] = localPos[0];
	localIntersectionPosition[1] = localPos[1];
	localIntersectionPosition[2] = localPos[2];

	// Actors position must be computed in world coordinates.
	actorX->SetPosition(localPos);
	actorX->Modified();
	actorY->SetPosition(localPos);
	actorY->Modified();
	actorZ->SetPosition(localPos);
	actorZ->Modified();
}

void DICOMVisualizer::setCTSliceIntersectionPointLocal(double localPos[3])
{
	// store parameters
	localIntersectionPosition[0] = localPos[0];
	localIntersectionPosition[1] = localPos[1];
	localIntersectionPosition[2] = localPos[2];

	
	// for computation of the slices origin
	// for ITK mesh the localPos's x-value must be flipped
	// for PHILIPS mesh the localPos's y-value must be flipped
	// see constructor and setCTInputFile
	if (!PhilipsMesh)
	{
		localPos[0] = -localPos[0];
	}
	else
	{
		localPos[1] = -localPos[1];
	}

	// Slices origin must be computed relative to the registration.
	double orig[3];
	for (int i = 0; i < 3; ++i) {
		orig[i] = (localPos[i] - volumePosition[i]);

	}

	for (int i = 0; i < 3; ++i) {
		reslices[i]->SetResliceAxesOrigin(orig);
		reslices[i]->Update();
	}

	// Now the flipping must be undone, because the world coordinate axes
	// are not flipped
	if (!PhilipsMesh)
	{
		localPos[0] = -localPos[0];
	}
	else
	{
		localPos[1] = -localPos[1];
	}

	actorX->SetPosition(localPos);
	actorX->Modified();
	actorY->SetPosition(localPos);
	actorY->Modified();
	actorZ->SetPosition(localPos);
	actorZ->Modified();

}

void DICOMVisualizer::getSliceIntersectionPointLocal(double localPos[3])
{
	localPos[0] = localIntersectionPosition[0];
	localPos[1] = localIntersectionPosition[1];
	localPos[2] = localIntersectionPosition[2];
}

void DICOMVisualizer::SetDICOMRenderer3D(vtkRenderer* renderer)
{
	dicomRenderer3D = renderer;
}


void DICOMVisualizer::SetDICOMRenderers2D(vtkRenderer* renderers[3])
{
	for (int i = 0; i < 3; i++)
	{
		dicomRenderers2D[i] = renderers[i];

	}
}

vtkRenderer* DICOMVisualizer::GetDICOMRenderers2D(unsigned int number) const
{
	if (number < 0 || number >= 3) return NULL;
	return dicomRenderers2D[number];
}

//#endif /*COMPILE_WITH_GDCM*/
