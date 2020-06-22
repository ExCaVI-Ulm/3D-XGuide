#include "Panel.h"
#include <OverlayScene.h>
#include <vtkMath.h> 
#include <vtkImageLuminance.h>
#include <vtkImageData.h>

using namespace cv;
using namespace std;


Panel::Panel(bool rao, bool kaud, int raoLao, int kaudKran, int lon, int lat, int vert, int sid, int fd)
	: rao_(rao), kaud_(kaud), raoLao_(raoLao), kaudKran_(kaudKran), lon_(lon), lat_(lat), vert_(vert), sid_(sid), fd_(fd)
{
	setupPattern();
}

//Panel::~Panel()
//{
//	// TODO: Delete 
//}

void Panel::setupPattern() 
{
	//================================================
	// Initialize pipeline for getGeometry
	//================================================
	//--------------------------TablePosition/RAOLAO/KRANKAUD-----------------------
	// RAOLAO
	LAORAO_FirstRect = cv::Rect(220, 1023 - 209, 19, 30);
	LAORAO_SecRect = cv::Rect(200, 1023 - 209, 19, 30);
	// KAUDKRAN
	KAUDKRAN_FirstRect = cv::Rect(220, 1023 - 258, 19, 30);
	KAUDKRAN_SecRect = cv::Rect(200, 1023 - 258, 19, 30);
	//LAORAO[0] = 0; LAORAO[1] = 90; KAUDKRAN[0] = KAUDKRAN[1] = 0;
	//RAO[0] = RAO[1] = true;	KAUD[0] = KAUD[1] = true;

	//--------------------------grad °-----------------------
	DEGREESIGN = Point(245, 845);
	FONTINK_RAOLAO = Point(15, 843);
	FONTINK_KAUDKRAN = Point(26, 794);

	//--------------------------Long----------------------- same pitch as RAOLAO, but always negativ, min = -98		
	//Long[0] = Long[1] = SystemGeometryDefinitions::TABLE_POSITION_Y; // evtl mit Iso initialisieren?
																	 //--------------------------Lat----------------------- same pitch as KAUDKRAN, in [-28,8]	
	KAUDKRAN_ThirdRect = cv::Rect(180, 1023 - 258, 19, 30);
	//Lat[0] = Lat[1] = SystemGeometryDefinitions::TABLE_POSITION_X; Lat_Sec[0] = Lat_Sec[1] = 0; Lat_Third[0] = Lat_Third[1] = 0; // evtl mit Iso initialisieren?

																																 //--------------------------Hoehe-----------------------
	Hoehe_FirstRect = cv::Rect(220, 1023 - 307, 19, 30);
	Hoehe_SecRect = cv::Rect(200, 1023 - 307, 19, 30);
	Hoehe_ThirdRect = cv::Rect(180, 1023 - 307, 19, 30);
	//Hoehe[0] = Hoehe[1] = SystemGeometryDefinitions::TABLE_POSITION_Z; Hoehe_Sec[0] = Hoehe_Sec[1] = 0; Hoehe_Third[0] = Hoehe_Third[1] = 0; // evtl mit Iso initialisieren?

																																			 //--------------------------SID-----------------------
	SID_FirstRect = cv::Rect(220, 1023 - 356, 19, 30);
	SID_SecRect = cv::Rect(200, 1023 - 356, 19, 30);
	SID_ThirdRect = cv::Rect(180, 1023 - 356, 19, 30);
	//SID[0] = 1200;
	//SID[1] = 1300;

	//--------------------------FD-----------------------
	FD_FirstRect = cv::Rect(220, 1023 - 405, 19, 30);
	FD_SecRect = cv::Rect(200, 1023 - 405, 19, 30);
	//FD[0] = FD[1] = 0;

}

// recognition and membering of values currently displayed
void Panel::update(vtkImageData* image) 
{
	bool rao, kaud;
	int raoLao, kaudKran, lon, lat, vert, sid, fd;
	bool raoN, kaudN;
	int raoLaoN, kaudKranN, lonN, latN, vertN, sidN, fdN;

	// get "old" values
	this->getDisplayedValues(rao, kaud, raoLao, kaudKran, lon, lat, vert, sid, fd);
	
	// recognize and set values currently displayed 
	this->recognizeDisplayedValues(image);

	// get "new" values
	this->getDisplayedValues(raoN, kaudN, raoLaoN, kaudKranN, lonN, latN, vertN, sidN, fdN);

	// compare old and new values, if there is any change the pipeline has to be updated
	if (rao != raoN || kaud != kaudN || raoLao != raoLaoN || kaudKran != kaudKranN || 
		lon != lonN || lat != latN || vert != vertN || sid != sidN || fd != fdN) 
	{
		this->setGeometryHasChanged(true);	
	}
	else 
	{
		this->setGeometryHasChanged(false);
	}


	if (fd != fdN)
	{
		this->setFDHasChanged(true);
	}
	else
	{
		this->setFDHasChanged(false);
	}

	if (lon != lonN || lat != latN || vert != vertN)
	{
		this->setTablePosHasChanged(true);
	}
	else
	{
		this->setTablePosHasChanged(false);
	}
}

void Panel::recognizeDisplayedValues(vtkImageData* image) 
{
	// declaration
	bool rao, kaud, angulation;

	cv::Mat LAORAO_FirstDigit_bin, LAORAO_SecDigit_bin;
	int LAORAO_FirstNorm, LAORAO_SecNorm;
	cv::Mat KAUDKRAN_FirstDigit_bin, KAUDKRAN_SecDigit_bin, KAUDKRAN_ThirdDigit_bin;
	int KAUDKRAN_FirstNorm, KAUDKRAN_SecNorm, KAUDKRAN_ThirdNorm;

	int LAORAO, KAUDKRAN, Long, Lat, Lat_Sec, Lat_Third;

	cv::Mat Hoehe_FirstDigit_bin, Hoehe_SecDigit_bin, Hoehe_ThirdDigit_bin;
	int Hoehe_FirstNorm, Hoehe_SecNorm, Hoehe_ThirdNorm;
	int Hoehe, Hoehe_Sec, Hoehe_Third;

	cv::Mat SID_FirstDigit_bin, SID_SecDigit_bin, SID_ThirdDigit_bin;
	int SID_FirstNorm, SID_SecNorm, SID_ThirdNorm;
	int SID;

	cv::Mat FD_FirstDigit_bin, FD_SecDigit_bin;
	int FD_FirstNorm, FD_SecNorm;
	int FD;

	double minVal, maxVal;
	// declaration-END-

	int dimensions[3];
	image->GetDimensions(dimensions);
	
	// Get a pointer to the actual pixel data. 
	//unsigned char *pVtkPix = (unsigned char *)image->GetScalarPointer();

	int widthIm = dimensions[0];
	int heightIm = dimensions[1];

	// create OpenCV matrix of the vtkImageData without(!) copying the content
	cv::Mat img(heightIm, widthIm, CV_8U, image->GetScalarPointer());

	//// Region of interest
	//Rect region_of_interest = Rect(x, y, w, h);

	//--------------------------TablePosition/RAOLAO/KRANKAUD-----------------------
	// RAOLAO first
	cv::minMaxLoc(img(LAORAO_FirstRect), &minVal, &maxVal);
	// checks for marker points? -> different threshold in RAO/LAO, KRAN/KAUD, SID
	double thresh;
	int threshtype;
	int ColorFont;
	if (minVal < 50) 
	{
		thresh = minVal;
		threshtype = 0;
		ColorFont = minVal;
	}
	else 
	{// marker points
		thresh = maxVal - 1;
		threshtype = 1;
		ColorFont = maxVal;
	}
	cv::threshold(img(LAORAO_FirstRect), LAORAO_FirstDigit_bin, thresh, 255, threshtype);
	LAORAO_FirstNorm = norm(LAORAO_FirstDigit_bin);
	// hier test ob ueberhaupt was angezeigt wird !!!
	//CR(LAORAO_FirstNorm[index])


	//namedWindow("Display window", WINDOW_NORMAL);// Create a window for display.
	//imshow("Display window", LAORAO_FirstDigit[0]);
	//namedWindow("Display window 2", WINDOW_NORMAL);// Create a window for display.
	//imshow("Display window 2", LAORAO_FirstDigit_bin[0]);

	//-----------------------------------------------------

	// RAOLAO second
	cv::threshold(img(LAORAO_SecRect), LAORAO_SecDigit_bin, thresh, 255, threshtype);
	LAORAO_SecNorm = norm(LAORAO_SecDigit_bin);

	// KAUDKRAN first
	cv::threshold(img(KAUDKRAN_FirstRect), KAUDKRAN_FirstDigit_bin, thresh, 255, threshtype);
	KAUDKRAN_FirstNorm = norm(KAUDKRAN_FirstDigit_bin);

	// KAUDKRAN second
	cv::threshold(img(KAUDKRAN_SecRect), KAUDKRAN_SecDigit_bin, thresh, 255, threshtype);
	KAUDKRAN_SecNorm = norm(KAUDKRAN_SecDigit_bin);

	//checks for °; wether angulation or table position is displayed 
	if ((int)img.at<uchar>(DEGREESIGN) == ColorFont) 
	{ 
		angulation = true;
		//--------------------------RAOLAO-----------------------
		// LAO || RAO ?
		(int)img.at<uchar>(FONTINK_RAOLAO) != ColorFont ? rao = false : rao = true;

		LAORAO = CR(LAORAO_FirstNorm) + 10 * CR(LAORAO_SecNorm);

		// KRAN-> + ; KAUD -> -
		LAORAO += (-2 * rao * LAORAO);

		//--------------------------KAUDKRAN---------------------
		// KRAN || KAUD ?
		(int)img.at<uchar>(FONTINK_KAUDKRAN) == ColorFont ? kaud = false : kaud = true;

		KAUDKRAN = CR(KAUDKRAN_FirstNorm) + 10 * CR(KAUDKRAN_SecNorm);

		// KRAN-> + ; KAUD -> -
		KAUDKRAN += (-2 * kaud * KAUDKRAN);

	}
	else
	{
		//--------------------------Tischpos-----------------------
		angulation = false;
		//--------------------------Long----------------------- same pitch as RAOLAO, but always negativ, min = -98		
		Long = (CR(LAORAO_FirstNorm) + 10 * CR(LAORAO_SecNorm))*-1;
		Long *= 10; // Anzeige im cm, jedoch hier in mm

		//--------------------------Lat----------------------- same pitch as KAUDKRAN, in [-28,8]	
		// KAUDKRAN Third
		cv::threshold(img(KAUDKRAN_ThirdRect), KAUDKRAN_ThirdDigit_bin, minVal, 255, cv::THRESH_BINARY);
		KAUDKRAN_ThirdNorm = norm(KAUDKRAN_ThirdDigit_bin);

		//Lat = 0;
		Lat = CR(KAUDKRAN_FirstNorm);
		Lat_Sec = CR(KAUDKRAN_SecNorm);
		Lat_Third = CR(KAUDKRAN_ThirdNorm);
		(Lat_Sec == -1) ? Lat *= -1 : Lat += 10 * Lat_Sec;		 // if 2. digit == minus 
		(Lat_Third == -1) ? Lat *= -1 : Lat += 10 * Lat_Third;   // if 3. digit == minus

		Lat *= 10; // Anzeige im cm, jedoch hier in mm
	}
	//--------------------------SID-----------------------
	// SID first
	cv::threshold(img(SID_FirstRect), SID_FirstDigit_bin, thresh, 255, threshtype);
	SID_FirstNorm = norm(SID_FirstDigit_bin);

	// SID second
	cv::threshold(img(SID_SecRect), SID_SecDigit_bin, thresh, 255, threshtype);
	SID_SecNorm = norm(SID_SecDigit_bin);

	// SID Third
	cv::threshold(img(SID_ThirdRect), SID_ThirdDigit_bin, thresh, 255, threshtype);
	SID_ThirdNorm = norm(SID_ThirdDigit_bin);
	// Anzeige im cm, jedoch hier in mm
	SID = (CR(SID_FirstNorm) + 10 * CR(SID_SecNorm) + 100 * CR(SID_ThirdNorm)) * 10; 

	// font of FD and Hoehe stays black even if marker point is reached
	//--------------------------FD-----------------------
	// FD first
	cv::minMaxLoc(img(FD_FirstRect), &minVal, &maxVal);
	cv::threshold(img(FD_FirstRect), FD_FirstDigit_bin, minVal, 255, cv::THRESH_BINARY);
	FD_FirstNorm = norm(FD_FirstDigit_bin);

	// FD second
	cv::threshold(img(FD_SecRect), FD_SecDigit_bin, minVal, 255, cv::THRESH_BINARY);
	FD_SecNorm = norm(FD_SecDigit_bin);

	FD = CR(FD_FirstNorm) + 10 * CR(FD_SecNorm);

	//--------------------------Hoehe-----------------------
	// Hoehe first
	cv::threshold(img(Hoehe_FirstRect), Hoehe_FirstDigit_bin, minVal, 255, cv::THRESH_BINARY);
	Hoehe_FirstNorm = norm(Hoehe_FirstDigit_bin);

	// Hoehe second
	cv::threshold(img(Hoehe_SecRect), Hoehe_SecDigit_bin, minVal, 255, cv::THRESH_BINARY);
	Hoehe_SecNorm = norm(Hoehe_SecDigit_bin);

	// Hoehe Third
	cv::threshold(img(Hoehe_ThirdRect), Hoehe_ThirdDigit_bin, minVal, 255, cv::THRESH_BINARY);
	Hoehe_ThirdNorm = norm(Hoehe_ThirdDigit_bin);

	// Hoehe = 0;
	Hoehe = CR(Hoehe_FirstNorm);
	Hoehe_Sec = CR(Hoehe_SecNorm);
	Hoehe_Third = CR(Hoehe_ThirdNorm);

	(Hoehe_Sec == -1) ? Hoehe *= -1 : Hoehe += 10 * Hoehe_Sec;		 // if 2. digit == minus 
	(Hoehe_Third == -1) ? Hoehe *= -1 : Hoehe += 10 * Hoehe_Third;   // if 3. digit == minus

	Hoehe *= 10; // Anzeige im cm, jedoch hier in mm

	// SID == 0 -> does not make sense most likely wrong input-> do not use this recognition
	if (SID != 0) 
	{
		this->setDisplayedValues(rao, kaud, angulation, (double)LAORAO, (double)KAUDKRAN, Long, Lat, Hoehe, SID, FD);
	}
	else 
	{
		cout << "ERROR: SID is zero. Most likely wrong input. No use of character recognition." << endl;
	}
	//image->Delete();

}

int Panel::CR(int characterNorm) {

	switch (characterNorm) {
		// binary linux HKL3, HKL4
		case 4898: return 1;
		case 4784: return 2;
		case 4858: return 3;
		case 4489: return 4;
		case 4632: return 5;
		case 4708: return 6;
		case 5055: return 7;
		case 4504: return 8;
		case 4681: return 9;
		case 4518: return 0;
		case 5457: return 0; // + ^= x+ 0*10 = X -> X bleibt positiv
		case 5870: return -1; // - !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		case 6088: return 0; // empty ^= Null
		default: return 0;
			break;
	}
}


void Panel::setDisplayedValues(bool rao, bool kaud, bool angulation, int raoLao, int kaudKran, int lon, int lat, int vert, int sid, int fd)
{
	if (angulation) 
	{
		rao_ = rao;
		kaud_ = kaud;
		raoLao_ = raoLao;
		kaudKran_ = kaudKran;
	} 
	else 
	{
		lon_ = lon;
		lat_ = lat;
	}
	vert_ = vert;
	sid_ = sid;
	fd_ = fd;
}

void Panel::getDisplayedValues(bool &rao, bool &kaud, int &raoLao, int &kaudKran, int &lon, int &lat, int &vert, int &sid, int &fd) 
{
	rao =  rao_;
	kaud = kaud_;
	raoLao = raoLao_;
	kaudKran =  kaudKran_;
	lon = lon_;
	lat = lat_;
	vert = vert_;
	sid = sid_;
	fd = fd_;
}

bool Panel::getGeometryHasChanged()
{
	return geometryHasChanged_;
}

void Panel::setGeometryHasChanged(bool geometryHasChanged) 
{
	geometryHasChanged_ = geometryHasChanged;
}

bool Panel::getFDHasChanged()
{
	return fdHasChanged_;
}

void Panel::setFDHasChanged(bool fdHasChanged)
{
	fdHasChanged_ = fdHasChanged;
}

void Panel::getTablePos(int &lon, int &lat, int &vert)
{
	lon = lon_;
	lat = lat_;
	vert = vert_;
}

bool Panel::getTablePosHasChanged()
{
	return tablePosHasChanged_;
}

void Panel::setTablePosHasChanged(bool tablePosHasChanged)
{
	tablePosHasChanged_ = tablePosHasChanged;
}

void Panel::getFirstAngle(bool &rao, int &laorao)
{
	rao = rao_;
	laorao = raoLao_;
}