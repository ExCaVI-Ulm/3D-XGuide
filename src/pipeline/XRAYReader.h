#ifndef XRAYREADER_H_
#define XRAYREADER_H_


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

#include <string>
#include <vector>

//using namespace std;

class XRAYReader : public vtkImageReader2
{
public:

	static XRAYReader* New() { return new XRAYReader(); }

	// -- connect through vtkGdcmImageReader methods
	vtkImageData* GetOutput() { return theXRAYReader->GetOutput(); };
	vtkAlgorithmOutput* GetOutputPort() { return theXRAYReader->GetOutputPort(); };
	void Update() { theXRAYReader->Update(); };

	int getNumberOfFrames();

	void setInputFile(const char* fileName);
	const char* getInputFile();

	double GetCineFrame();
	int GetECGBeatFrame();
	int GetSID(bool* valid);
	int GetSourcePatientDistance(bool* valid);
	double GetPrimaryAngle(bool* valid);
	double GetSecondaryAngle(bool* valid);
	double GetMillimeterPerPixelScaling(bool* valid);
	double GetTableTopVerticalPosition(bool* valid);
	double GetTableTopLongitudinalPosition(bool* valid);
	double GetTableTopLateralPosition(bool* valid);
	int GetImageSizeX(bool* valid);
	int GetImageSizeY(bool* valid);

	void ReadGeometryFramegrabber(double& primAngleRead, double& secAngleRead, int& longRead, int& latRead, int& HoeheRead, int& SIDRead, double& mmPerPxlRead, int& FDRead, int& SODRead);
	void ReadGeometryFramegrabberAngle(double& primAngleRead, double& secAngleRead);
	void ReadGeometryFramegrabberTable(int& longRead, int& latRead, int& HoeheRead);
	void ReadGeometryFramegrabberSID(int& SIDRead);
	void ReadGeometryFramegrabbermmPerPxl(double& mmPerPxlRead);
	void ReadGeometryFramegrabberFD(int& FDRead);
	void ReadGeometryFramegrabberSPD(int& SPDread);

protected:
	XRAYReader();
	~XRAYReader();

private:

	vtkGDCMImageReader* theXRAYReader;	
	std::string theFilename;

};

#endif /* XRAYREADER_H_ */

