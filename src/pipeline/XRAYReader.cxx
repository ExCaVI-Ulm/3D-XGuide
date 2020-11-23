#include "XRAYReader.h"


#include <vtkImageShiftScale.h>
#include <vtkImageReslice.h>
#include <vtkImageActor.h>
#include <vtkAssembly.h>

#include <gdcmReader.h>
#include <gdcmFile.h>
#include <gdcmTag.h>
#include <gdcmStringFilter.h>


#include "gdcmGlobal.h"
#include "gdcmDicts.h"
#include "gdcmDict.h"
#include "gdcmAttribute.h"

#include <gdcmPixmapReader.h>


#include <stdlib.h>

XRAYReader::XRAYReader()
{
	theXRAYReader = vtkGDCMImageReader::New();

}

XRAYReader::~XRAYReader()
{
	theXRAYReader->Delete();
}

void XRAYReader::setInputFile(const char* fn)
{
	theFilename.clear();
	theFilename.append(fn);
	theXRAYReader->SetFileName(fn);
};

const char* XRAYReader::getInputFile()
{
	return theFilename.c_str();
}



int XRAYReader::getNumberOfFrames()
{
	int extends[6];
	theXRAYReader->GetDataExtent(extends);
	return extends[5] + 1;
}

double XRAYReader::GetCineFrame()
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	double frame = -1.0;

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve Cine frame information from file!" << std::endl;
		return -1;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		return -1;
	}


	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;
	dicomStringFilter->SetFile(dicomFile);

	//Tag is according to DICOM Header (ImageJ)
	gdcm::Tag tag = gdcm::Tag(0x0018, 0x0040);
	std::string strSID = dicomStringFilter->ToString(tag);
	frame = atoi(strSID.c_str());
	if (frame == 8.0)	// in  DICOM header thefalue is rounded!!!
	{
		frame = 7.5;
	}
	//delete(dicomStringFilter);	//TODO
	delete(dicomReader);

	return frame;
}


int XRAYReader::GetECGBeatFrame()
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	int ecg = -1;

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve SID information from file!" << std::endl;
		return -1;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		return -1;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;
	
	dicomStringFilter->SetFile(dicomFile);

	//Tag is according to DICOM Header (ImageJ)
	gdcm::Tag tag = gdcm::Tag(0x5000, 0x0114);
	std::string strSID = dicomStringFilter->ToString(tag);
	ecg = atoi(strSID.c_str());

	//delete(dicomStringFilter);	//TODO
	delete(dicomReader);

	return ecg;
}


void XRAYReader::ReadGeometryFramegrabber(double& primAngleRead, double& secAngleRead, int& longRead, int& latRead, int& HoeheRead, int& SIDRead, double& mmPerPxlRead, int& FDRead, int& SODRead)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());
	const char* description; // for data grabbed with python script long Table position is written to tag(0x0008, 0x0050), modality is XA and implementation version name is PYDICOM, for 3D-XGuide - (0x0020, 0x0011), CT, and VTK_DICOM

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve SID information from file!" << std::endl;
		return;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		return;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;

	dicomStringFilter->SetFile(dicomFile);

	//Tag is according to DICOM Header (ImageJ)

	gdcm::Tag tag = gdcm::Tag(0x0002, 0x0013);
	std::string strSID = dicomStringFilter->ToString(tag);
	description = strSID.c_str();
	if (description[0] == 'V')
	{
		tag = gdcm::Tag(0x0020, 0x0011);	
	}
	else
	{
		tag = gdcm::Tag(0x0008, 0x0050);

	}
	strSID = dicomStringFilter->ToString(tag);
	longRead = atoi(strSID.c_str());


	tag = gdcm::Tag(0x0008, 0x0020);
	strSID = dicomStringFilter->ToString(tag);
	primAngleRead = atoi(strSID.c_str());

	tag = gdcm::Tag(0x0008, 0x0030);
	strSID = dicomStringFilter->ToString(tag);
	secAngleRead = atoi(strSID.c_str());

	//tag = gdcm::Tag(0x0020, 0x0011);	
	/*tag = gdcm::Tag(0x0008, 0x0050);
	strSID = dicomStringFilter->ToString(tag);
	longRead = atoi(strSID.c_str());*/

	tag = gdcm::Tag(0x0020, 0x0012);
	strSID = dicomStringFilter->ToString(tag);
	latRead = atoi(strSID.c_str());

	tag = gdcm::Tag(0x0020, 0x0010);
	strSID = dicomStringFilter->ToString(tag);
	HoeheRead = atoi(strSID.c_str());

	tag = gdcm::Tag(0x0018, 0x1110);
	strSID = dicomStringFilter->ToString(tag);
	SIDRead = atoi(strSID.c_str());

	tag = gdcm::Tag(0x0028, 0x0030);
	strSID = dicomStringFilter->ToString(tag);
	mmPerPxlRead = atof(strSID.c_str()); //TODO

	tag = gdcm::Tag(0x0020, 0x1040);
	strSID = dicomStringFilter->ToString(tag);
	FDRead = atoi(strSID.c_str()); //TODO

	tag = gdcm::Tag(0x0018, 0x0060);
	strSID = dicomStringFilter->ToString(tag);
	SODRead = atoi(strSID.c_str());

	//delete(dicomStringFilter);	//TODO
	delete(dicomReader);
}

void XRAYReader::ReadGeometryFramegrabberAngle(double& primAngleRead, double& secAngleRead) {

	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());


	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve SID information from file!" << std::endl;
		return;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		return;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;

	dicomStringFilter->SetFile(dicomFile);

	//Tag is according to DICOM Header (ImageJ)
	gdcm::Tag tag = gdcm::Tag(0x0008, 0x0020);
	std::string strprimAngleRead = dicomStringFilter->ToString(tag);
	primAngleRead = atoi(strprimAngleRead.c_str());

	tag = gdcm::Tag(0x0008, 0x0030);
	std::string strsecAngleRead = dicomStringFilter->ToString(tag);
	secAngleRead = atoi(strsecAngleRead.c_str());
	
	delete(dicomReader);
}


void XRAYReader::ReadGeometryFramegrabberTable(int& longRead, int& latRead, int& HoeheRead)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());
	const char* description; // for data grabbed with python script long Table position is written to tag(0x0008, 0x0050), for vtk - (0x0020, 0x0011)

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve SID information from file!" << std::endl;
		return;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		return;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;

	dicomStringFilter->SetFile(dicomFile);

	gdcm::Tag tag = gdcm::Tag(0x0002, 0x0013);
	std::string strTable = dicomStringFilter->ToString(tag);
	description = strTable.c_str();
	if (description[0] == 'V')
	{
		tag = gdcm::Tag(0x0020, 0x0011);
	}
	else
	{
		tag = gdcm::Tag(0x0008, 0x0050);

	}
	strTable = dicomStringFilter->ToString(tag);
	longRead = atoi(strTable.c_str());


	//Tag is according to DICOM Header (ImageJ)
	//gdcm::Tag tag = gdcm::Tag(0x0020, 0x0011);
	/*tag = gdcm::Tag(0x0008, 0x0050);
	strTable = dicomStringFilter->ToString(tag);
	longRead = atoi(strTable.c_str());*/

	tag = gdcm::Tag(0x0020, 0x0012);
	strTable = dicomStringFilter->ToString(tag);
	latRead = atoi(strTable.c_str());

	tag = gdcm::Tag(0x0020, 0x0010);
	strTable = dicomStringFilter->ToString(tag);
	HoeheRead = atoi(strTable.c_str());

	delete(dicomReader);
}

void XRAYReader::ReadGeometryFramegrabberSID(int& SIDRead)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve SID information from file!" << std::endl;
		return;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		return;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;

	dicomStringFilter->SetFile(dicomFile);

	//Tag is according to DICOM Header (ImageJ)
	gdcm::Tag tag = gdcm::Tag(0x0018, 0x1110);
	std::string strSID  = dicomStringFilter->ToString(tag);
	SIDRead = atoi(strSID.c_str());

	delete(dicomReader);
}

void XRAYReader::ReadGeometryFramegrabbermmPerPxl(double& mmPerPxlRead)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve SID information from file!" << std::endl;
		return;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		return;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;

	dicomStringFilter->SetFile(dicomFile);

	//Tag is according to DICOM Header (ImageJ)
	gdcm::Tag  tag = gdcm::Tag(0x0028, 0x0030);
	std::string strmmPerPxl = dicomStringFilter->ToString(tag);
	mmPerPxlRead = atof(strmmPerPxl.c_str()); 

	delete(dicomReader);
}

void XRAYReader::ReadGeometryFramegrabberFD(int& FDRead)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve SID information from file!" << std::endl;
		return;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		return;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;

	dicomStringFilter->SetFile(dicomFile);

	//Tag is according to DICOM Header (ImageJ)
	gdcm::Tag tag = gdcm::Tag(0x0020, 0x1040);
	std::string strFD = dicomStringFilter->ToString(tag);
	FDRead = atoi(strFD.c_str()); //TODO

	delete(dicomReader);
}

void XRAYReader::ReadGeometryFramegrabberImageSizeX(int& ImageSizeXRead)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve image size(columns) from file!" << std::endl;
		return;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		return;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;

	dicomStringFilter->SetFile(dicomFile);

	//Tag is according to DICOM Header (ImageJ)
	gdcm::Tag tag = gdcm::Tag(0x0028, 0x0011);
	std::string strSizeX = dicomStringFilter->ToString(tag);
	ImageSizeXRead = atoi(strSizeX.c_str()); //TODO

	delete(dicomReader);
}

void XRAYReader::ReadGeometryFramegrabberImageSizeY(int& ImageSizeYRead)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve image size(rows) from file!" << std::endl;
		return;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		return;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;

	dicomStringFilter->SetFile(dicomFile);

	//Tag is according to DICOM Header (ImageJ)
	gdcm::Tag tag = gdcm::Tag(0x0028, 0x0010);
	std::string strSizeY = dicomStringFilter->ToString(tag);
	ImageSizeYRead = atoi(strSizeY.c_str()); //TODO

	delete(dicomReader);
}


void XRAYReader::ReadGeometryFramegrabberSPD(int& SPDread)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve SID information from file!" << std::endl;
		return;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		return;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;

	dicomStringFilter->SetFile(dicomFile);

	//Tag is according to DICOM Header (ImageJ)
	gdcm::Tag  tag = gdcm::Tag(0x0018, 0x0060);
	std::string strSPD = dicomStringFilter->ToString(tag);
	SPDread = atof(strSPD.c_str());

	delete(dicomReader);
}

int XRAYReader::GetSID(bool* valid)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	int sid = -1;

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve SID information from file!" << std::endl;
		*valid = false; return -1;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		*valid = false; return -1;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;
	dicomStringFilter->SetFile(dicomFile);

	//Tag is according to DICOM Header (ImageJ)
	gdcm::Tag tag = gdcm::Tag(0x0018, 0x1110);
	std::string strSID = dicomStringFilter->ToString(tag);
	sid = atoi(strSID.c_str());

	//delete(dicomStringFilter);	//TODO
	delete(dicomReader);

	*valid = true;
	return sid;
}

int XRAYReader::GetSourcePatientDistance(bool* valid)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	int pid = -1;

	if (!dicomReader->CanRead())
	{
		cout << "DICOM header is not readable! " <<
			"Can not retrieve PID information from file!" << endl;
		*valid = false; return -1;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		cerr << "cannot read DICOM!" << endl;
		*valid = false; return -1;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;
	dicomStringFilter->SetFile(dicomFile);

	gdcm::Tag tag = gdcm::Tag(0x0018, 0x1111);
	std::string strPID = dicomStringFilter->ToString(tag);
	pid = atoi(strPID.c_str());


	//delete(dicomStringFilter); //TODO
	delete(dicomReader);

	*valid = true;
	return pid;
}

double XRAYReader::GetPrimaryAngle(bool* valid)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	double angle = 0.0;

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve primary angle information from file!" << std::endl;
		*valid = false; return -1;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		*valid = false; return -1;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;
	dicomStringFilter->SetFile(dicomFile);

	gdcm::Tag tag = gdcm::Tag(0x0018, 0x1510);
	std::string strAngle = dicomStringFilter->ToString(tag);
	angle = atof(strAngle.c_str());

	//delete(dicomStringFilter);	//TODO
	delete(dicomReader);

	*valid = true;
	return angle;
}

double XRAYReader::GetSecondaryAngle(bool* valid)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	double angle = 0.0;

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve secondary angle information from file!" << std::endl;
		*valid = false; return -1;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		*valid = false; return -1;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;
	dicomStringFilter->SetFile(dicomFile);

	gdcm::Tag tag = gdcm::Tag(0x0018, 0x1511);
	std::string strAngle = dicomStringFilter->ToString(tag);
	angle = atof(strAngle.c_str());

	//delete(dicomStringFilter);	//TODO
	delete(dicomReader);

	*valid = true;
	return angle;
}

double XRAYReader::GetMillimeterPerPixelScaling(bool* valid)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	double MillimeterperPixel = 0.0;

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve image pixel scaling information from file!" << std::endl;
		*valid = false; return -1;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		*valid = false; return -1;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;
	dicomStringFilter->SetFile(dicomFile);

	gdcm::Tag tag = gdcm::Tag(0x0018, 0x1164);
	std::string strMM = dicomStringFilter->ToString(tag);
	MillimeterperPixel = atof(strMM.c_str());
	
	//delete(dicomStringFilter);	//TODO
	delete(dicomReader);

	*valid = true;
	return MillimeterperPixel;
}

double XRAYReader::GetTableTopVerticalPosition(bool* valid)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	double verticalPos = 0.0;

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve vertical table top position information from file!" << std::endl;
		*valid = false; return -1;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		*valid = false; return -1;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;
	dicomStringFilter->SetFile(dicomFile);


	// the dataset is the set of elements we are interested in:
	gdcm::DataSet &ds = dicomFile.GetDataSet();
	//gdcm::Tag tag = gdcm::Tag(0x300A, 0x0128);
	// table position is at nested tag------------------------------------
	gdcm::Tag tag = gdcm::Tag(0x2003, 0x102E);
	gdcm::Tag subtag = gdcm::Tag(0x300A, 0x0128);
	
	
	const gdcm::DataElement &seq = ds.GetDataElement(tag);
	gdcm::SequenceOfItems* sqi = seq.GetValueAsSQ();
	if(sqi == NULL) {
		std::cerr << "cannot read nested Table Top Position Tag!" << std::endl;
		*valid = false; return -1;
	};
	if (sqi->GetNumberOfItems() == 0) {
		std::cerr << "cannot read nested Table Top Position Tag!" << std::endl; 
		*valid = false; return -1;
	};
	gdcm::Item &item = sqi->GetItem(1);

	gdcm::DataSet &subds = item.GetNestedDataSet();

	if (!subds.FindDataElement(subtag)) {
		std::cerr << "cannot find nested Table Top Position Tag!" << std::endl;
		*valid = false; return -1;
	}

	const gdcm::DataElement &de = item.GetDataElement(subtag);
	const gdcm::ByteValue * value = de.GetByteValue();

	char *buffer;
	gdcm::VL vl = value->GetLength();
	uint32_t length = (uint32_t)vl;
	buffer = new char[length + 1];
	value->GetBuffer(buffer, length);
	buffer[length] = 0;

	verticalPos = (double)atof(buffer);
	delete buffer;


	delete(dicomReader);

	*valid = true;
	return verticalPos;
}

double XRAYReader::GetTableTopLongitudinalPosition(bool* valid)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	double longitudinalPos = 0.0;

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve vertical table top position information from file!" << std::endl;
		*valid = false; return -1;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		*valid = false; return -1;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;
	dicomStringFilter->SetFile(dicomFile);

	// table position is at nested tag-----------------------------------
	gdcm::DataSet &ds = dicomFile.GetDataSet();
	gdcm::Tag tag = gdcm::Tag(0x2003, 0x102E);
	gdcm::Tag subtag = gdcm::Tag(0x300A, 0x0129);


	const gdcm::DataElement &seq = ds.GetDataElement(tag);
	gdcm::SequenceOfItems* sqi = seq.GetValueAsSQ();
	if (sqi == NULL) {
		std::cerr << "cannot read nested Table Top Position Tag!" << std::endl;
		*valid = false; return -1;
	};
	if (sqi->GetNumberOfItems() == 0) {
		std::cerr << "cannot read nested Table Top Position Tag!" << std::endl;
		*valid = false; return -1;
	};
	gdcm::Item &item = sqi->GetItem(1);

	gdcm::DataSet &subds = item.GetNestedDataSet();

	if (!subds.FindDataElement(subtag)) {
		std::cerr << "cannot find nested Table Top Position Tag!" << std::endl;
		*valid = false; return -1;
	}

	const gdcm::DataElement &de = item.GetDataElement(subtag);
	const gdcm::ByteValue * value = de.GetByteValue();

	char *buffer;
	gdcm::VL vl = value->GetLength();
	uint32_t length = (uint32_t)vl;
	buffer = new char[length + 1];
	value->GetBuffer(buffer, length);
	buffer[length] = 0;

	longitudinalPos = (double)atof(buffer);
	delete buffer;

	//delete(dicomStringFilter);	//TODO
	delete(dicomReader);

	*valid = true;
	return longitudinalPos;
}

double XRAYReader::GetTableTopLateralPosition(bool* valid)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	double lateralPos = 0.0;

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve vertical table top position information from file!" << std::endl;
		*valid = false; return -1;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		*valid = false; return -1;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;
	dicomStringFilter->SetFile(dicomFile);

	// table position is at nested tag-----------------------------------
	gdcm::DataSet &ds = dicomFile.GetDataSet();
	gdcm::Tag tag = gdcm::Tag(0x2003, 0x102E);
	gdcm::Tag subtag = gdcm::Tag(0x300A, 0x012A);


	const gdcm::DataElement &seq = ds.GetDataElement(tag);
	gdcm::SequenceOfItems* sqi = seq.GetValueAsSQ();
	if (sqi == NULL) {
		std::cerr << "cannot read nested Table Top Position Tag!" << std::endl;
		*valid = false; return -1;
	};
	if (sqi->GetNumberOfItems() == 0) {
		std::cerr << "cannot read nested Table Top Position Tag!" << std::endl;
		*valid = false; return -1;
	};
	gdcm::Item &item = sqi->GetItem(1);

	gdcm::DataSet &subds = item.GetNestedDataSet();

	if (!subds.FindDataElement(subtag)) {
		std::cerr << "cannot find nested Table Top Position Tag!" << std::endl;
		*valid = false; return -1;
	}

	const gdcm::DataElement &de = item.GetDataElement(subtag);
	const gdcm::ByteValue * value = de.GetByteValue();

	char *buffer;
	gdcm::VL vl = value->GetLength();
	uint32_t length = (uint32_t)vl;
	buffer = new char[length + 1];
	value->GetBuffer(buffer, length);
	buffer[length] = 0;

	lateralPos = (double)atof(buffer);
	delete buffer;

	//delete(dicomStringFilter);	//TODO
	delete(dicomReader);

	*valid = true;
	return lateralPos;
}


int XRAYReader::GetImageSizeX(bool* valid)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	int imageSizeX = 0;

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve image size (columns) from file!" << std::endl;
		*valid = false; return -1;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		*valid = false; return -1;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;
	dicomStringFilter->SetFile(dicomFile);

	gdcm::Tag tag = gdcm::Tag(0x0028, 0x0011);
	std::string strImageSizeX = dicomStringFilter->ToString(tag);
	imageSizeX = atoi(strImageSizeX.c_str());

	//delete(dicomStringFilter);	//TODO
	delete(dicomReader);

	*valid = true;
	return imageSizeX;
}


int XRAYReader::GetImageSizeY(bool* valid)
{
	gdcm::Reader* dicomReader = new gdcm::Reader;
	dicomReader->SetFileName(theFilename.c_str());

	int imageSizeY = 0;

	if (!dicomReader->CanRead())
	{
		std::cout << "DICOM header is not readable! " <<
			"Can not retrieve image size (rows) from file!" << std::endl;
		*valid = false; return -1;
	}

	bool success = dicomReader->Read();
	if (!success)
	{
		std::cerr << "cannot read DICOM!" << std::endl;
		*valid = false; return -1;
	}

	gdcm::File dicomFile = dicomReader->GetFile();
	gdcm::StringFilter* dicomStringFilter = new gdcm::StringFilter;
	dicomStringFilter->SetFile(dicomFile);

	gdcm::Tag tag = gdcm::Tag(0x0028, 0x0010);
	std::string strImageSizeY = dicomStringFilter->ToString(tag);
	imageSizeY = atoi(strImageSizeY.c_str());

	//delete(dicomStringFilter);	//TODO
	delete(dicomReader);

	*valid = true;
	return imageSizeY;
}